#include "render/model.h"
#include "render/objcalc.h"
#include "core/shader.h"
#include <fstream>
#include <iterator>
#include <limits>
#ifdef _DEBUG
#include "core/debug_ostream.h"
#endif

struct MODEL
{
	ID3D11Buffer* VertexBuffer;
	UINT VertexCount;
	UINT TintStart; // ここから後ろが色替えする三角形（なければ VertexCount）
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

void ModelFinalize(void)
{
	for (MODEL& model : g_Models) SAFE_RELEASE(model.VertexBuffer);
	g_Models.clear();
}

int ModelLoad(const char* path, bool mirrorX, const char* tintMaterial)
{
	const auto fail = [path](const char* reason) {
#ifdef _DEBUG
		hal::dout << "ModelLoad：" << reason << "：" << (path ? path : "(null)") << std::endl;
#else
		(void)path;
		(void)reason;
#endif
		return -1;
	};
	if (!path) return fail("ファイルが開けない（パスなし）");
	if (!DirectXGetDevice()) return fail("バッファが作れない（デバイスなし）");
	bool ok = false;
	const std::string source = ReadFile(path, ok);
	if (!ok) return fail("ファイルが開けない");
	// MTL は OBJ と同じフォルダから読む。読めなければ材質なし（白）で描く。
	std::vector<OBJ_MATERIAL> materials;
	const std::string mtlName = ObjCalcGetMtlLib(source);
	if (!mtlName.empty()) {
		const std::string objPath(path);
		const size_t slash = objPath.find_last_of("/\\");
		const std::string mtlSource = ReadFile(
			(slash == std::string::npos ? std::string() : objPath.substr(0, slash + 1)) + mtlName, ok);
		if (!ok || !ObjCalcParseMtl(mtlSource, materials)) fail("MTL が読めない（白で描く）");
	}
	std::vector<OBJ_VERTEX> parsed;
	if (!ObjCalcParse(source, materials, parsed)) return fail("OBJ が読めない");
	if (mirrorX) ObjCalcMirrorX(parsed);
	if (parsed.size() > (std::numeric_limits<UINT>::max)() / sizeof(VERTEX_3D)
		|| g_Models.size() >= static_cast<size_t>((std::numeric_limits<int>::max)()))
		return fail("バッファが作れない（サイズ・モデル数の上限）");
	int tint = -1;
	for (size_t i = 0; tintMaterial && i < materials.size(); ++i) {
		if (materials[i].Name == tintMaterial) tint = static_cast<int>(i);
	}
	// 色替えする三角形は後ろにまとめ、ModelDrawTinted で別の色を掛けて描く。
	std::vector<VERTEX_3D> vertices, tinted;
	vertices.reserve(parsed.size());
	for (const OBJ_VERTEX& v : parsed) {
		const bool isTint = tint >= 0 && v.Material == tint;
		const VEC3 kd = v.Material >= 0 && !isTint ? materials[v.Material].Diffuse : VEC3{ 1, 1, 1 };
		(isTint ? tinted : vertices).push_back({ { v.Position.X, v.Position.Y, v.Position.Z },
			{ v.Normal.X, v.Normal.Y, v.Normal.Z }, { kd.X, kd.Y, kd.Z, 1 }, { 0, 0 } });
	}
	MODEL model{};
	model.TintStart = static_cast<UINT>(vertices.size());
	vertices.insert(vertices.end(), tinted.begin(), tinted.end());
	model.VertexCount = static_cast<UINT>(vertices.size());
	D3D11_BUFFER_DESC desc{};
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = static_cast<UINT>(sizeof(VERTEX_3D) * vertices.size());
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA data{};
	data.pSysMem = vertices.data();
	if (FAILED(DirectXGetDevice()->CreateBuffer(&desc, &data, &model.VertexBuffer)))
		return fail("バッファが作れない");
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

static void DrawRange(UINT start, UINT count, const XMFLOAT4& color)
{
	if (count == 0) return;
	MATERIAL material{};
	material.Diffuse = color;
	material.Ambient = color;
	Shader_SetMaterial(material);
	DirectXGetDeviceContext()->Draw(count, start);
}

void ModelDraw(int model, const XMFLOAT3& origin, const XMFLOAT3& xAxis,
	const XMFLOAT3& yAxis, const XMFLOAT3& zAxis, const XMFLOAT4& color)
{
	const MODEL* mesh = SetupDraw(model, origin, xAxis, yAxis, zAxis);
	if (mesh) DrawRange(0, mesh->VertexCount, color);
}

void ModelDrawTinted(int model, const XMFLOAT3& origin, const XMFLOAT3& xAxis,
	const XMFLOAT3& yAxis, const XMFLOAT3& zAxis, const XMFLOAT4& tint)
{
	const MODEL* mesh = SetupDraw(model, origin, xAxis, yAxis, zAxis);
	if (!mesh) return;
	DrawRange(0, mesh->TintStart, { 1.0f, 1.0f, 1.0f, tint.w });
	DrawRange(mesh->TintStart, mesh->VertexCount - mesh->TintStart, tint);
}
