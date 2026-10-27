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
};

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

int ModelLoad(const char* path, bool mirrorX)
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
	std::ifstream file(path, std::ios::binary);
	if (!file) return fail("ファイルが開けない");
	const std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::vector<OBJ_VERTEX> parsed;
	if (file.bad() || !ObjCalcParse(source, parsed)) return fail("OBJ が読めない");
	if (mirrorX) ObjCalcMirrorX(parsed);
	if (parsed.size() > (std::numeric_limits<UINT>::max)() / sizeof(VERTEX_3D)
		|| g_Models.size() >= static_cast<size_t>((std::numeric_limits<int>::max)()))
		return fail("バッファが作れない（サイズ・モデル数の上限）");
	std::vector<VERTEX_3D> vertices;
	vertices.reserve(parsed.size());
	for (const OBJ_VERTEX& v : parsed) {
		vertices.push_back({ { v.Position.X, v.Position.Y, v.Position.Z },
			{ v.Normal.X, v.Normal.Y, v.Normal.Z }, { 1, 1, 1, 1 }, { 0, 0 } });
	}
	MODEL model{};
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

void ModelDraw(int model, const XMFLOAT3& origin, const XMFLOAT3& xAxis,
	const XMFLOAT3& yAxis, const XMFLOAT3& zAxis, const XMFLOAT4& color)
{
	if (model < 0 || static_cast<size_t>(model) >= g_Models.size()) return;
	const MODEL& mesh = g_Models[model];
	const XMMATRIX world(xAxis.x, xAxis.y, xAxis.z, 0,
		yAxis.x, yAxis.y, yAxis.z, 0, zAxis.x, zAxis.y, zAxis.z, 0,
		origin.x, origin.y, origin.z, 1);
	Shader_SetWorldMatrix(world);
	MATERIAL material{};
	material.Diffuse = color;
	material.Ambient = color;
	Shader_SetMaterial(material);
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	DirectXGetDeviceContext()->IASetVertexBuffers(0, 1, &mesh.VertexBuffer, &stride, &offset);
	DirectXGetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DirectXGetDeviceContext()->Draw(mesh.VertexCount, 0);
}
