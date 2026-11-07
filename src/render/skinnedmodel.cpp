#include "render/skinnedmodel.h"
#include "core/debug_ostream.h"
#include "core/shader.h"
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <string>
#include <unordered_map>

namespace {
	struct Node
	{
		int Parent;
		ANIM_TRANSFORM BindPose;
	};

	struct Bone
	{
		size_t NodeIndex;
		ANIM_MATRIX Offset;
	};

	struct Mesh
	{
		ID3D11Buffer* Vertices = nullptr;
		ID3D11Buffer* Indices = nullptr;
		UINT IndexCount = 0;
		std::vector<Bone> Bones;
	};

	struct Clip
	{
		float LoopSeconds = 0;
		std::vector<ANIM_TRACK> Tracks;
	};
}

static std::vector<Node> g_Nodes;
static std::vector<Mesh> g_Meshes;
static std::array<Clip, static_cast<size_t>(SKINNED_CLIP::Count)> g_Clips;
static bool g_LoadAttempted = false;
static constexpr unsigned WEIGHTS_PER_VERTEX = 4;
static constexpr unsigned IMPORT_FLAGS = aiProcess_ConvertToLeftHanded | aiProcess_LimitBoneWeights
	| aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices;

using NodeNames = std::unordered_map<std::string, size_t>;

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

static void ReadNodes(const aiNode& source, int parent, NodeNames& names, std::vector<size_t>& meshNodes)
{
	const size_t index = g_Nodes.size();
	names.emplace(source.mName.C_Str(), index);
	aiVector3D scale, position;
	aiQuaternion rotation;
	source.mTransformation.Decompose(scale, rotation, position);
	g_Nodes.push_back({ parent, { ToAnim(position), ToAnim(rotation), ToAnim(scale) } });
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

static bool ReadMesh(const aiMesh& source, size_t meshNode, const NodeNames& names, Mesh& mesh)
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

static bool ReadClip(const aiScene& scene, const NodeNames& names, Clip& clip)
{
	if (scene.mNumAnimations != 1) return false;
	const aiAnimation& animation = *scene.mAnimations[0];
	if (animation.mTicksPerSecond <= 0.0 || animation.mDuration <= 0.0) return false;
	// 末尾キーは先頭と同じ姿勢。次の周と重ね、末尾の1フレームぶんは待たない。
	clip.LoopSeconds = static_cast<float>(animation.mDuration / animation.mTicksPerSecond);
	clip.Tracks.resize(g_Nodes.size());
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

static bool Load(const std::array<const char*, static_cast<size_t>(SKINNED_CLIP::Count)>& paths)
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
	g_Meshes.resize(scene->mNumMeshes);
	for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
		if (!ReadMesh(*scene->mMeshes[i], meshNodes[i], names, g_Meshes[i])) return false;
	}
	if (!ReadClip(*scene, names, g_Clips[0])) return false;
	for (size_t i = 1; i < paths.size(); ++i) {
		scene = importer.ReadFile(paths[i], IMPORT_FLAGS);
		if (!scene || !ReadClip(*scene, names, g_Clips[i])) {
			hal::dout << paths[i] << ": " << importer.GetErrorString() << std::endl;
			return false;
		}
	}
	return true;
}

bool SkinnedModelLoad(const std::array<const char*, static_cast<size_t>(SKINNED_CLIP::Count)>& paths)
{
	if (!g_LoadAttempted && !Load(paths)) {
		hal::dout << "SkinnedModelLoad failed: " << paths[0] << std::endl;
		SkinnedModelFinalize();
	}
	g_LoadAttempted = true;
	return !g_Meshes.empty();
}

void SkinnedModelFinalize()
{
	for (Mesh& mesh : g_Meshes) {
		SAFE_RELEASE(mesh.Indices);
		SAFE_RELEASE(mesh.Vertices);
	}
	g_Meshes.clear();
	g_Nodes.clear();
	g_Clips = {};
	g_LoadAttempted = false;
}

static void SamplePose(SKINNED_POSE& pose, float blend)
{
	const Clip& clip = g_Clips[static_cast<size_t>(pose.Clip)];
	for (size_t i = 0; i < g_Nodes.size(); ++i) {
		const Node& node = g_Nodes[i];
		const ANIM_TRANSFORM next = AnimCalcSample(clip.Tracks[i], pose.Time, node.BindPose);
		pose.Local[i] = blend < 1.0f ? AnimCalcBlend(pose.BlendFrom[i], next, blend) : next;
		const ANIM_MATRIX local = AnimCalcMatrix(pose.Local[i]);
		pose.Global[i] = node.Parent < 0 ? local : AnimCalcCompose(local, pose.Global[node.Parent]);
	}
}

void SkinnedModelReset(SKINNED_POSE& pose)
{
	pose.Clip = SKINNED_CLIP::Idle;
	pose.Time = 0.0f;
	pose.BlendElapsed = 0.0f;
	pose.BlendFrom.clear();
	pose.Local.resize(g_Nodes.size());
	pose.Global.resize(g_Nodes.size());
	SamplePose(pose, 1.0f);
}

void SkinnedModelUpdate(SKINNED_POSE& pose, SKINNED_CLIP clip, float dt, float blendSeconds)
{
	if (g_Nodes.empty()) return;
	if (pose.Clip != clip) {
		// 切り替え途中でも、直前に表示した姿勢から次の補間を始める。
		pose.BlendFrom = pose.Local;
		pose.Clip = clip;
		pose.Time = 0.0f;
		pose.BlendElapsed = 0.0f;
	}
	pose.Time = AnimCalcLoopTime(pose.Time + dt, g_Clips[static_cast<size_t>(clip)].LoopSeconds);
	float blend = 1.0f;
	if (!pose.BlendFrom.empty()) {
		pose.BlendElapsed += dt;
		blend = AnimCalcBlendWeight(pose.BlendElapsed, blendSeconds);
		if (blend >= 1.0f) pose.BlendFrom.clear();
	}
	SamplePose(pose, blend);
}

void SkinnedModelDraw(const SKINNED_POSE& pose, const XMMATRIX& world, const XMFLOAT4& color)
{
	Shader_BeginSkinned();
	Shader_SetWorldMatrix(world);
	MATERIAL material{};
	material.Diffuse = material.Ambient = color;
	Shader_SetMaterial(material);

	ID3D11DeviceContext* context = DirectXGetDeviceContext();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	const UINT stride = sizeof(SKINNED_VERTEX), offset = 0;
	for (const Mesh& mesh : g_Meshes) {
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
