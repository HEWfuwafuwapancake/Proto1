#include "render/model.h"
#include "render/objcalc.h"
#include "render/texture.h"
#include "core/shader.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <limits>
#pragma warning(push, 0)
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#pragma warning(pop)
#ifdef _DEBUG
#include "core/debug_ostream.h"
#endif

// 同じ色・同じテクスチャで続けて描ける三角形のまとまり
struct MODEL_PART
{
	UINT Start; // 頂点バッファの何番目から
	UINT Count; // 頂点の数
	ID3D11ShaderResourceView* Texture; // 貼るテクスチャ（なければ nullptr。MODEL::Textures が持つ）
	bool Tinted; // ModelDrawTinted で色替えする部分か
};

struct MODEL
{
	ID3D11Buffer* VertexBuffer;
	UINT VertexCount;
	std::vector<MODEL_PART> Parts; // 色替えする部分は後ろにまとめる
	std::vector<ID3D11ShaderResourceView*> Textures; // このモデルが読み込んだテクスチャ（Finalize で解放する）
	std::string Path; // 読み込んだときの引数（同じ引数で呼ばれたら、このモデルを使い回す）
	bool MirrorX;
	std::string TintMaterial;
};

static std::string ReadFile(const std::string& path, bool& ok)
{
	std::ifstream file(path, std::ios::binary);
	ok = static_cast<bool>(file);
	if (!ok) return {};
	std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	ok = !file.bad();
	return source;
}

static std::vector<MODEL> g_Models;

void ModelInitialize(void)
{
	ModelFinalize();
}

static void ReleaseModel(MODEL& model)
{
	SAFE_RELEASE(model.VertexBuffer);
	for (ID3D11ShaderResourceView*& texture : model.Textures) SAFE_RELEASE(texture);
	model.Textures.clear();
}

void ModelFinalize(void)
{
	for (MODEL& model : g_Models) ReleaseModel(model);
	g_Models.clear();
}

static void PrintFail(const char* path, const char* reason)
{
#ifdef _DEBUG
	hal::dout << "ModelLoad：" << reason << "：" << (path ? path : "(null)") << std::endl;
#else
	(void)path;
	(void)reason;
#endif
}

// パスの拡張子が .obj か（大文字・小文字は区別しない）
static bool IsObjPath(const std::string& path)
{
	if (path.size() < 4) return false;
	std::string ext = path.substr(path.size() - 4);
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return ext == ".obj";
}

// パスのフォルダ部分（最後の / か \ まで。なければ空）
static std::string FolderOf(const std::string& path)
{
	const size_t slash = path.find_last_of("/\\");
	return slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
}

// 自作の OBJ 読み込み。MTL の Kd を頂点色にし、色替えする材質の三角形は後ろにまとめる。
static bool LoadObj(const char* path, bool mirrorX, const char* tintMaterial, std::vector<VERTEX_3D>& vertices,
	std::vector<MODEL_PART>& parts)
{
	bool ok = false;
	const std::string source = ReadFile(path, ok);
	if (!ok) {
		PrintFail(path, "ファイルが開けない");
		return false;
	}
	// MTL は OBJ と同じフォルダから読む。読めなければ材質なし（白）で描く。
	std::vector<OBJ_MATERIAL> materials;
	const std::string mtlName = ObjCalcGetMtlLib(source);
	if (!mtlName.empty()) {
		const std::string mtlSource = ReadFile(FolderOf(path) + mtlName, ok);
		if (!ok || !ObjCalcParseMtl(mtlSource, materials)) PrintFail(path, "MTL が読めない（白で描く）");
	}
	std::vector<OBJ_VERTEX> parsed;
	if (!ObjCalcParse(source, materials, parsed)) {
		PrintFail(path, "OBJ が読めない");
		return false;
	}
	if (mirrorX) ObjCalcMirrorX(parsed);
	int tint = -1;
	for (size_t i = 0; tintMaterial && i < materials.size(); ++i) {
		if (materials[i].Name == tintMaterial) tint = static_cast<int>(i);
	}
	std::vector<VERTEX_3D> tinted;
	vertices.reserve(parsed.size());
	for (const OBJ_VERTEX& v : parsed) {
		const bool isTint = tint >= 0 && v.Material == tint;
		const VEC3 kd = v.Material >= 0 && !isTint ? materials[v.Material].Diffuse : VEC3{ 1, 1, 1 };
		(isTint ? tinted : vertices).push_back({ { v.Position.X, v.Position.Y, v.Position.Z },
			{ v.Normal.X, v.Normal.Y, v.Normal.Z }, { kd.X, kd.Y, kd.Z, 1 }, { 0, 0 } });
	}
	const UINT tintStart = static_cast<UINT>(vertices.size());
	vertices.insert(vertices.end(), tinted.begin(), tinted.end());
	parts.push_back({ 0, tintStart, nullptr, false });
	parts.push_back({ tintStart, static_cast<UINT>(vertices.size()) - tintStart, nullptr, true });
	return true;
}

// 材質の色のテクスチャを読む。FBX に埋め込まれていればそれを使い、なければモデルと同じフォルダから読む。
static ID3D11ShaderResourceView* LoadMaterialTexture(const aiScene* scene, const aiMaterial* material,
	const std::string& folder, const char* path)
{
	aiString name;
	if (aiGetMaterialTexture(material, aiTextureType_DIFFUSE, 0, &name) != AI_SUCCESS
		&& aiGetMaterialTexture(material, aiTextureType_BASE_COLOR, 0, &name) != AI_SUCCESS)
		return nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	if (const aiTexture* embedded = scene->GetEmbeddedTexture(name.C_Str())) {
		// mHeight が 0 なら PNG などのファイルの中身（mWidth がバイト数）、そうでなければ B・G・R・A の画素
		view = embedded->mHeight == 0
			? TextureLoadFromMemory(embedded->pcData, embedded->mWidth)
			: TextureCreateBGRA(embedded->pcData, embedded->mWidth, embedded->mHeight);
	} else {
		const std::string file(name.C_Str());
		view = TextureLoadFromFile((folder + file).c_str());
		// 書き出した PC のフォルダ名が付いていることがあるので、ファイル名だけでも探す
		const size_t slash = file.find_last_of("/\\");
		if (!view && slash != std::string::npos) view = TextureLoadFromFile((folder + file.substr(slash + 1)).c_str());
	}
	if (!view) PrintFail(path, "テクスチャが読めない（材質の色で描く）");
	return view;
}

// Assimp で FBX などを読む。ノードの移動・回転・拡大は頂点に焼き込み、Z を反転して左手系にする（OBJ と同じ向き）。
static bool LoadWithAssimp(const char* path, bool mirrorX, const char* tintMaterial, std::vector<VERTEX_3D>& vertices,
	std::vector<MODEL_PART>& parts, std::vector<ID3D11ShaderResourceView*>& textures)
{
	const aiScene* scene = aiImportFile(path, aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_FlipUVs
		| aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals | aiProcess_SortByPType);
	if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
#ifdef _DEBUG
		hal::dout << "ModelLoad：Assimp：" << aiGetErrorString() << std::endl;
#endif
		PrintFail(path, "モデルが読めない");
		if (scene) aiReleaseImport(scene);
		return false;
	}
	const std::string folder = FolderOf(path);
	// 材質ごとのテクスチャ（同じ材質を使うメッシュで読み直さない）と、色替えする材質か
	std::vector<ID3D11ShaderResourceView*> materialTextures(scene->mNumMaterials, nullptr);
	std::vector<bool> materialTinted(scene->mNumMaterials, false);
	for (unsigned int m = 0; m < scene->mNumMaterials; ++m) {
		aiString materialName;
		aiGetMaterialString(scene->mMaterials[m], AI_MATKEY_NAME, &materialName);
		materialTinted[m] = tintMaterial && materialName == aiString(tintMaterial);
		materialTextures[m] = LoadMaterialTexture(scene, scene->mMaterials[m], folder, path);
		if (materialTextures[m]) textures.push_back(materialTextures[m]);
	}
	// 色替えしない部分を先に、色替えする部分を後ろに並べる
	for (int pass = 0; pass < 2; ++pass) {
		for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
			const aiMesh* mesh = scene->mMeshes[i];
			const unsigned int matIndex = mesh->mMaterialIndex;
			const bool tinted = matIndex < scene->mNumMaterials && materialTinted[matIndex];
			if (tinted != (pass == 1) || !(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) || !mesh->HasNormals()) continue;
			ID3D11ShaderResourceView* texture = matIndex < scene->mNumMaterials ? materialTextures[matIndex] : nullptr;
			// テクスチャを貼る部分と色替えする部分は白（テクスチャ・指定色をそのまま出す）。ほかは材質の色。
			aiColor4D kd(1, 1, 1, 1);
			if (!texture && !tinted && matIndex < scene->mNumMaterials)
				aiGetMaterialColor(scene->mMaterials[matIndex], AI_MATKEY_COLOR_DIFFUSE, &kd);
			const UINT start = static_cast<UINT>(vertices.size());
			for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
				const aiFace& face = mesh->mFaces[f];
				if (face.mNumIndices != 3) continue;
				for (unsigned int k = 0; k < 3; ++k) {
					const unsigned int v = face.mIndices[k];
					const aiVector3D& p = mesh->mVertices[v];
					const aiVector3D& n = mesh->mNormals[v];
					const aiVector3D uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][v] : aiVector3D(0, 0, 0);
					vertices.push_back({ { p.x, p.y, p.z }, { n.x, n.y, n.z }, { kd.r, kd.g, kd.b, 1 }, { uv.x, uv.y } });
				}
			}
			const UINT count = static_cast<UINT>(vertices.size()) - start;
			if (count == 0) continue;
			// 前のまとまりと色・テクスチャが同じなら、1回で描けるようにつなげる
			if (!parts.empty() && parts.back().Texture == texture && parts.back().Tinted == tinted)
				parts.back().Count += count;
			else
				parts.push_back({ start, count, texture, tinted });
		}
	}
	aiReleaseImport(scene);
	if (vertices.empty()) {
		PrintFail(path, "三角形がない");
		return false;
	}
	if (mirrorX) {
		for (VERTEX_3D& v : vertices) {
			v.Position.x = -v.Position.x;
			v.Normal.x = -v.Normal.x;
		}
		// 鏡写しで回る向きが逆になるので、2番目と3番目を入れ替えて時計回りに戻す。
		for (size_t i = 0; i + 2 < vertices.size(); i += 3) std::swap(vertices[i + 1], vertices[i + 2]);
	}
	return true;
}

int ModelLoad(const char* path, bool mirrorX, const char* tintMaterial)
{
	if (!path) {
		PrintFail(path, "ファイルが開けない（パスなし）");
		return -1;
	}
	if (!DirectXGetDevice()) {
		PrintFail(path, "バッファが作れない（デバイスなし）");
		return -1;
	}
	// 同じ引数で読み込み済みなら、読み直さずに同じ番号を返す（腕は人数 × 左右 × 種類の数だけ同じファイルを読むため）
	const std::string tintName = tintMaterial ? tintMaterial : "";
	for (size_t i = 0; i < g_Models.size(); ++i) {
		const MODEL& loaded = g_Models[i];
		if (loaded.Path == path && loaded.MirrorX == mirrorX && loaded.TintMaterial == tintName) return static_cast<int>(i);
	}
	MODEL model{};
	model.Path = path;
	model.MirrorX = mirrorX;
	model.TintMaterial = tintName;
	// .obj は今までどおり自作の読み込み、それ以外（.fbx など）は Assimp で読む
	std::vector<VERTEX_3D> vertices;
	const bool loaded = IsObjPath(path)
		? LoadObj(path, mirrorX, tintMaterial, vertices, model.Parts)
		: LoadWithAssimp(path, mirrorX, tintMaterial, vertices, model.Parts, model.Textures);
	if (!loaded || vertices.size() > (std::numeric_limits<UINT>::max)() / sizeof(VERTEX_3D)
		|| g_Models.size() >= static_cast<size_t>((std::numeric_limits<int>::max)())) {
		if (loaded) PrintFail(path, "バッファが作れない（サイズ・モデル数の上限）");
		ReleaseModel(model);
		return -1;
	}
	model.VertexCount = static_cast<UINT>(vertices.size());
	D3D11_BUFFER_DESC desc{};
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = static_cast<UINT>(sizeof(VERTEX_3D) * vertices.size());
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA data{};
	data.pSysMem = vertices.data();
	if (vertices.empty() || FAILED(DirectXGetDevice()->CreateBuffer(&desc, &data, &model.VertexBuffer))) {
		PrintFail(path, "バッファが作れない");
		ReleaseModel(model);
		return -1;
	}
	g_Models.push_back(model);
	return static_cast<int>(g_Models.size() - 1);
}

static const MODEL* SetupDraw(int model, const XMFLOAT3& origin, const XMFLOAT3& xAxis,
	const XMFLOAT3& yAxis, const XMFLOAT3& zAxis)
{
	if (model < 0 || static_cast<size_t>(model) >= g_Models.size()) return nullptr;
	const MODEL& mesh = g_Models[model];
	const XMMATRIX world(xAxis.x, xAxis.y, xAxis.z, 0,
		yAxis.x, yAxis.y, yAxis.z, 0, zAxis.x, zAxis.y, zAxis.z, 0,
		origin.x, origin.y, origin.z, 1);
	Shader_SetWorldMatrix(world);
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	DirectXGetDeviceContext()->IASetVertexBuffers(0, 1, &mesh.VertexBuffer, &stride, &offset);
	DirectXGetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	return &mesh;
}

// まとまりごとに色とテクスチャを設定して描く（tinted が nullptr なら全部 color、あれば色替えする部分だけ tinted の色）
static void DrawParts(const MODEL& mesh, const XMFLOAT4& color, const XMFLOAT4* tinted)
{
	ID3D11DeviceContext* context = DirectXGetDeviceContext();
	bool textureSet = false;
	for (const MODEL_PART& part : mesh.Parts) {
		if (part.Count == 0) continue;
		const XMFLOAT4 partColor = !tinted ? color : part.Tinted ? *tinted : XMFLOAT4{ 1.0f, 1.0f, 1.0f, tinted->w };
		MATERIAL material{};
		material.Diffuse = partColor;
		material.Ambient = partColor;
		material.TextureEnable = part.Texture ? TRUE : FALSE;
		Shader_SetMaterial(material);
		if (part.Texture) {
			context->PSSetShaderResources(0, 1, &part.Texture);
			textureSet = true;
		}
		context->Draw(part.Count, part.Start);
	}
	if (textureSet) {
		ID3D11ShaderResourceView* empty = nullptr;
		context->PSSetShaderResources(0, 1, &empty);
	}
}

void ModelDraw(int model, const XMFLOAT3& origin, const XMFLOAT3& xAxis,
	const XMFLOAT3& yAxis, const XMFLOAT3& zAxis, const XMFLOAT4& color)
{
	const MODEL* mesh = SetupDraw(model, origin, xAxis, yAxis, zAxis);
	if (mesh) DrawParts(*mesh, color, nullptr);
}

void ModelDrawTinted(int model, const XMFLOAT3& origin, const XMFLOAT3& xAxis,
	const XMFLOAT3& yAxis, const XMFLOAT3& zAxis, const XMFLOAT4& tint)
{
	const MODEL* mesh = SetupDraw(model, origin, xAxis, yAxis, zAxis);
	if (mesh) DrawParts(*mesh, tint, &tint);
}
