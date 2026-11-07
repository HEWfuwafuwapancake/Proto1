#include "render/skinnedmodel.h"
#include "core/debug_ostream.h"
#include "core/shader.h"
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <algorithm>
#include <cstring>

static constexpr unsigned WEIGHTS_PER_VERTEX = 4;
static constexpr size_t REQUIRED_CLIPS = static_cast<size_t>(SKINNED_CLIP::RunBack); // 待機・走り・左腕・右腕

// ループしないクリップ（最後の姿勢で止める）
static bool IsLoopClip(SKINNED_CLIP clip)
{
	return clip != SKINNED_CLIP::Jump && clip != SKINNED_CLIP::Land;
}

// ファイルがないクリップの代わり
static SKINNED_CLIP FallbackClip(SKINNED_CLIP clip)
{
	return clip == SKINNED_CLIP::RunBack ? SKINNED_CLIP::Run : SKINNED_CLIP::Idle;
}
static constexpr unsigned IMPORT_FLAGS = aiProcess_ConvertToLeftHanded | aiProcess_LimitBoneWeights
	| aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices;

static ANIM_MATRIX ToRowMatrix(const aiMatrix4x4& m)
{
	// Assimp の列ベクトル用の行列を、行ベクトル用に転置する。
	return {{{ m.a1, m.b1, m.c1, m.d1 }, { m.a2, m.b2, m.c2, m.d2 },
		{ m.a3, m.b3, m.c3, m.d3 }, { m.a4, m.b4, m.c4, m.d4 }}};
}

static ANIM_VEC3 ToAnim(const aiVector3D& value)
{
	return { value.x, value.y, value.z };
}

static ANIM_QUAT ToAnim(aiQuaternion value)
{
	value.Normalize();
	return { value.x, value.y, value.z, value.w };
}

void SkinnedModel::ReadNodes(const aiNode& source, int parent, NodeNames& names, std::vector<size_t>& meshNodes)
{
	const size_t index = m_Nodes.size();
	names.emplace(source.mName.C_Str(), index);
	aiVector3D scale, position;
	aiQuaternion rotation;
	source.mTransformation.Decompose(scale, rotation, position);
	m_Nodes.push_back({ parent, { ToAnim(position), ToAnim(rotation), ToAnim(scale) } });
	for (unsigned i = 0; i < source.mNumMeshes; ++i) meshNodes[source.mMeshes[i]] = index;
	for (unsigned i = 0; i < source.mNumChildren; ++i) {
		ReadNodes(*source.mChildren[i], static_cast<int>(index), names, meshNodes);
	}
}

template<class T>
static bool CreateBuffer(const std::vector<T>& data, UINT bind, ID3D11Buffer** buffer)
{
	if (data.empty() || data.size() > UINT_MAX / sizeof(T)) return false;
	D3D11_BUFFER_DESC desc{};
	desc.ByteWidth = static_cast<UINT>(data.size() * sizeof(T));
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = bind;
	D3D11_SUBRESOURCE_DATA initial{};
	initial.pSysMem = data.data();
	return SUCCEEDED(DirectXGetDevice()->CreateBuffer(&desc, &initial, buffer));
}

bool SkinnedModel::ReadMesh(const aiMesh& source, size_t meshNode, const NodeNames& names, Mesh& mesh)
{
	if (!source.HasNormals() || source.mNumBones > SHADER_MAX_BONES) return false;
	std::vector<SKINNED_VERTEX> vertices(source.mNumVertices);
	std::vector<unsigned> weightCounts(source.mNumVertices);
	for (unsigned i = 0; i < source.mNumVertices; ++i) {
		const aiVector3D& p = source.mVertices[i];
		const aiVector3D& n = source.mNormals[i];
		vertices[i].Position = { p.x, p.y, p.z };
		vertices[i].Normal = { n.x, n.y, n.z };
	}
	for (unsigned i = 0; i < source.mNumBones; ++i) {
		const aiBone& bone = *source.mBones[i];
		const auto node = names.find(bone.mName.C_Str());
		if (node == names.end()) return false;
		mesh.Bones.push_back({ node->second, ToRowMatrix(bone.mOffsetMatrix) });
		for (unsigned j = 0; j < bone.mNumWeights; ++j) {
			const aiVertexWeight& weight = bone.mWeights[j];
			if (weight.mVertexId >= vertices.size()) return false;
			const unsigned slot = weightCounts[weight.mVertexId]++;
			if (slot >= WEIGHTS_PER_VERTEX) return false;
			SKINNED_VERTEX& vertex = vertices[weight.mVertexId];
			vertex.Bones[slot] = i;
			vertex.Weights[slot] = weight.mWeight;
		}
	}
	const UINT rigidBone = static_cast<UINT>(mesh.Bones.size());
	bool needsRigidBone = false;
	for (SKINNED_VERTEX& vertex : vertices) {
		float sum = 0.0f;
		for (float weight : vertex.Weights) sum += weight;
		if (sum > 0.0f) {
			for (float& weight : vertex.Weights) weight /= sum;
		} else {
			vertex.Bones[0] = rigidBone;
			vertex.Weights[0] = 1.0f;
			needsRigidBone = true;
		}
	}
	// 重みがない頂点は、メッシュを持っているノードと一緒に動かす。
	if (needsRigidBone) {
		if (rigidBone >= SHADER_MAX_BONES) return false;
		mesh.Bones.push_back({ meshNode, AnimCalcMatrix({}) });
	}

	std::vector<UINT> indices;
	indices.reserve(static_cast<size_t>(source.mNumFaces) * 3);
	for (unsigned i = 0; i < source.mNumFaces; ++i) {
		const aiFace& face = source.mFaces[i];
		if (face.mNumIndices != 3) continue;
		indices.insert(indices.end(), face.mIndices, face.mIndices + 3);
	}
	mesh.IndexCount = static_cast<UINT>(indices.size());
	return CreateBuffer(vertices, D3D11_BIND_VERTEX_BUFFER, &mesh.Vertices)
		&& CreateBuffer(indices, D3D11_BIND_INDEX_BUFFER, &mesh.Indices);
}

template<class T, class Key>
static void ReadKeys(std::vector<ANIM_KEY<T>>& out, const Key* keys, unsigned count, double ticksPerSecond)
{
	out.reserve(count);
	for (unsigned i = 0; i < count; ++i) {
		out.push_back({ static_cast<float>(keys[i].mTime / ticksPerSecond), ToAnim(keys[i].mValue) });
	}
}

bool SkinnedModel::ReadClip(const aiScene& scene, const NodeNames& names, Clip& clip) const
{
	const aiAnimation* selected = nullptr;
	for (unsigned i = 0; i < scene.mNumAnimations; ++i) {
		const aiAnimation* candidate = scene.mAnimations[i];
		if (scene.mNumAnimations == 1 || std::strcmp(candidate->mName.C_Str(), "Take 001") != 0) {
			selected = candidate;
			break;
		}
	}
	if (!selected) return false;
	const aiAnimation& animation = *selected;
	if (animation.mTicksPerSecond <= 0.0 || animation.mDuration <= 0.0) return false;
	// 末尾キーは先頭と同じ姿勢。次の周と重ね、末尾の1フレームぶんは待たない。
	clip.LoopSeconds = static_cast<float>(animation.mDuration / animation.mTicksPerSecond);
	clip.Tracks.resize(m_Nodes.size());
	for (unsigned i = 0; i < animation.mNumChannels; ++i) {
		const aiNodeAnim& channel = *animation.mChannels[i];
		const auto node = names.find(channel.mNodeName.C_Str());
		if (node == names.end()) return false;
		ANIM_TRACK& track = clip.Tracks[node->second];
		ReadKeys(track.Positions, channel.mPositionKeys, channel.mNumPositionKeys, animation.mTicksPerSecond);
		ReadKeys(track.Rotations, channel.mRotationKeys, channel.mNumRotationKeys, animation.mTicksPerSecond);
		ReadKeys(track.Scales, channel.mScalingKeys, channel.mNumScalingKeys, animation.mTicksPerSecond);
	}
	return true;
}

void SkinnedModel::ReadBones(const NodeNames& names, const SKINNED_BONES_DESC& bones)
{
	m_UpperMask.clear();
	m_TwistNodes.clear();
	const auto upper = bones.UpperRoot ? names.find(bones.UpperRoot) : names.end();
	if (upper != names.end()) {
		// ノードは親が先に並ぶので、親が上半身なら子も上半身。
		m_UpperMask.resize(m_Nodes.size());
		for (size_t i = 0; i < m_Nodes.size(); ++i) {
			const int parent = m_Nodes[i].Parent;
			m_UpperMask[i] = i == upper->second || (parent >= 0 && m_UpperMask[parent]);
		}
	} else if (bones.UpperRoot) {
		hal::dout << "SkinnedModel: upper root not found: " << bones.UpperRoot << std::endl;
	}
	for (const char* name : bones.Twist) {
		if (!name) continue;
		const auto node = names.find(name);
		if (node != names.end()) m_TwistNodes.push_back(node->second);
		else hal::dout << "SkinnedModel: twist bone not found: " << name << std::endl;
	}
}

bool SkinnedModel::LoadFiles(const std::array<const char*, static_cast<size_t>(SKINNED_CLIP::Count)>& paths, const SKINNED_BONES_DESC& bones)
{
	Assimp::Importer importer;
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
	importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, WEIGHTS_PER_VERTEX);
	const aiScene* scene = importer.ReadFile(paths[0], IMPORT_FLAGS);
	if (!scene || !scene->mRootNode || !scene->HasMeshes()) {
		hal::dout << paths[0] << ": " << importer.GetErrorString() << std::endl;
		return false;
	}
	NodeNames names;
	std::vector<size_t> meshNodes(scene->mNumMeshes);
	ReadNodes(*scene->mRootNode, -1, names, meshNodes);
	m_Meshes.resize(scene->mNumMeshes);
	for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
		if (!ReadMesh(*scene->mMeshes[i], meshNodes[i], names, m_Meshes[i])) return false;
	}
	ReadBones(names, bones);
	m_NodeNames = names;
	if (!ReadClip(*scene, names, m_Clips[0])) return false;
	for (size_t i = 1; i < paths.size(); ++i) {
		if (paths[i]) {
			scene = importer.ReadFile(paths[i], IMPORT_FLAGS);
			if (scene && ReadClip(*scene, names, m_Clips[i])) continue;
			hal::dout << paths[i] << ": " << importer.GetErrorString() << std::endl;
		}
		if (i < REQUIRED_CLIPS) return false;
		// 後ろ走り・ジャンプなどのファイルがなければ、読み込み済みのクリップで代わりに動かす。
		const SKINNED_CLIP clip = static_cast<SKINNED_CLIP>(i);
		m_Clips[i] = GetClip(FallbackClip(clip));
		m_Clips[i].Reverse = clip == SKINNED_CLIP::RunBack;
	}
	return true;
}

bool SkinnedModel::Load(const std::array<const char*, static_cast<size_t>(SKINNED_CLIP::Count)>& paths, const SKINNED_BONES_DESC& bones)
{
	Finalize();
	if (LoadFiles(paths, bones)) return true;
	hal::dout << "SkinnedModel::Load failed: " << paths[0] << std::endl;
	Finalize();
	return false;
}

void SkinnedModel::Finalize()
{
	for (Mesh& mesh : m_Meshes) {
		SAFE_RELEASE(mesh.Indices);
		SAFE_RELEASE(mesh.Vertices);
	}
	m_Meshes.clear();
	m_Nodes.clear();
	m_Clips = {};
	m_UpperMask.clear();
	m_TwistNodes.clear();
	m_NodeNames.clear();
}

int SkinnedModel::FindNode(const char* name) const
{
	if (!name) return -1;
	const auto node = m_NodeNames.find(name);
	return node != m_NodeNames.end() ? static_cast<int>(node->second) : -1;
}

bool SkinnedModel::GetNodePosition(const SKINNED_POSE& pose, int node, ANIM_VEC3& position) const
{
	if (node < 0 || static_cast<size_t>(node) >= pose.Global.size() || pose.Global.size() != m_Nodes.size()) return false;
	const ANIM_MATRIX& global = pose.Global[node];
	position = { global[3][0], global[3][1], global[3][2] };
	return true;
}

ANIM_TRANSFORM SkinnedModel::SampleClip(SKINNED_CLIP clip, float time, size_t node) const
{
	const Clip& source = GetClip(clip);
	const float t = source.Reverse ? source.LoopSeconds - time : time;
	return AnimCalcSample(source.Tracks[node], t, m_Nodes[node].BindPose);
}

void SkinnedModel::SamplePose(SKINNED_POSE& pose, float blend) const
{
	const bool upper = pose.Upper != SKINNED_CLIP::Count && pose.UpperWeight > 0.0f && !m_UpperMask.empty();
	const float twist = m_TwistNodes.empty() ? 0.0f : pose.Twist / static_cast<float>(m_TwistNodes.size());
	for (size_t i = 0; i < m_Nodes.size(); ++i) {
		const Node& node = m_Nodes[i];
		const ANIM_TRANSFORM next = SampleClip(pose.Clip, pose.Time, i);
		ANIM_TRANSFORM local = blend < 1.0f ? AnimCalcBlend(pose.BlendFrom[i], next, blend) : next;
		// 上半身の骨だけ、腕のクリップを重みの分だけ重ねる（脚は走りなどのまま）。
		if (upper && m_UpperMask[i]) local = AnimCalcBlend(local, SampleClip(pose.Upper, pose.UpperTime, i), pose.UpperWeight);
		pose.Local[i] = local;
		const ANIM_MATRIX matrix = AnimCalcMatrix(local);
		pose.Global[i] = node.Parent < 0 ? matrix : AnimCalcCompose(matrix, pose.Global[node.Parent]);
		// ねじる骨は、ねじれを等分してモデルの上下の軸まわりに回す（子の骨もついてくる）。
		if (twist != 0.0f && std::find(m_TwistNodes.begin(), m_TwistNodes.end(), i) != m_TwistNodes.end()) {
			pose.Global[i] = AnimCalcRotateY(pose.Global[i], twist);
		}
	}
}

void SkinnedModel::Reset(SKINNED_POSE& pose) const
{
	pose.Clip = SKINNED_CLIP::Idle;
	pose.Time = 0.0f;
	pose.BlendElapsed = 0.0f;
	pose.Upper = SKINNED_CLIP::Count;
	pose.UpperTime = 0.0f;
	pose.UpperWeight = 0.0f;
	pose.Twist = 0.0f;
	pose.BlendFrom.clear();
	pose.Local.resize(m_Nodes.size());
	pose.Global.resize(m_Nodes.size());
	SamplePose(pose, 1.0f);
}

void SkinnedModel::Update(SKINNED_POSE& pose, const SKINNED_REQUEST& request, float dt, float blendSeconds) const
{
	if (!IsLoaded()) return;
	if (pose.Local.size() != m_Nodes.size() || pose.Global.size() != m_Nodes.size()) Reset(pose);
	const SKINNED_CLIP clip = request.Clip;
	if (pose.Clip != clip) {
		// 切り替え途中でも、直前に表示した姿勢から次の補間を始める。
		pose.BlendFrom = pose.Local;
		pose.Clip = clip;
		pose.Time = 0.0f;
		pose.BlendElapsed = 0.0f;
	}
	const float length = GetClip(clip).LoopSeconds;
	const float time = pose.Time + dt * (request.Speed > 0.0f ? request.Speed : 0.0f);
	pose.Time = IsLoopClip(clip) ? AnimCalcLoopTime(time, length) : std::min(time, length);

	// 上半身のクリップは、出し入れのときに重みを少しずつ変える。
	const bool wantUpper = request.Upper != SKINNED_CLIP::Count && !m_UpperMask.empty();
	if (wantUpper && pose.Upper != request.Upper) {
		pose.Upper = request.Upper;
		pose.UpperTime = 0.0f;
	}
	const float step = blendSeconds > 0.0f ? dt / blendSeconds : 1.0f;
	pose.UpperWeight = wantUpper ? std::min(1.0f, pose.UpperWeight + step) : std::max(0.0f, pose.UpperWeight - step);
	if (pose.UpperWeight <= 0.0f) pose.Upper = SKINNED_CLIP::Count;
	if (pose.Upper != SKINNED_CLIP::Count) {
		pose.UpperTime = AnimCalcLoopTime(pose.UpperTime + dt, GetClip(pose.Upper).LoopSeconds);
	}
	pose.Twist = request.Twist;
	float blend = 1.0f;
	if (!pose.BlendFrom.empty()) {
		pose.BlendElapsed += dt;
		blend = AnimCalcBlendWeight(pose.BlendElapsed, blendSeconds);
		if (blend >= 1.0f) pose.BlendFrom.clear();
	}
	SamplePose(pose, blend);
}

void SkinnedModel::Draw(const SKINNED_POSE& pose, const XMMATRIX& world, const XMFLOAT4& color) const
{
	if (!IsLoaded()) return;
	if (pose.Global.size() != m_Nodes.size()) {
		SKINNED_POSE idle;
		Reset(idle);
		Draw(idle, world, color);
		return;
	}
	Shader_BeginSkinned();
	Shader_SetWorldMatrix(world);
	MATERIAL material{};
	material.Diffuse = material.Ambient = color;
	Shader_SetMaterial(material);

	ID3D11DeviceContext* context = DirectXGetDeviceContext();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	const UINT stride = sizeof(SKINNED_VERTEX), offset = 0;
	for (const Mesh& mesh : m_Meshes) {
		SKINNED_BONES palette{};
		for (size_t i = 0; i < mesh.Bones.size(); ++i) {
			const Bone& bone = mesh.Bones[i];
			const ANIM_MATRIX transform = AnimCalcCompose(bone.Offset, pose.Global[bone.NodeIndex]);
			// HLSL の行列は列優先で格納する。
			for (int row = 0; row < 4; ++row) {
				for (int column = 0; column < 4; ++column) {
					palette.Matrices[i].m[column][row] = transform[row][column];
				}
			}
		}
		Shader_SetSkinnedBones(palette);
		context->IASetVertexBuffers(0, 1, &mesh.Vertices, &stride, &offset);
		context->IASetIndexBuffer(mesh.Indices, DXGI_FORMAT_R32_UINT, 0);
		context->DrawIndexed(mesh.IndexCount, 0, 0);
	}
	Shader_Begin();
}
