#include "core/dx11.h"
#include "core/shader.h"
#include "render/shape.h"
#include "physics/tentaclecalc.h"


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
static constexpr int SPHERE_SLICES = 24;	// 球・円柱を横に何分割するか（多いほど丸い）
static constexpr int SPHERE_STACKS = 16;	// 球を縦に何分割するか


// 形1つ分のバッファ
struct MESH
{
	ID3D11Buffer* VertexBuffer;	// 頂点バッファ
	ID3D11Buffer* IndexBuffer;	// インデックスバッファ
	UINT IndexCount;			// インデックスの数
};

static MESH g_Box = {};			// 1辺 1.0 の箱（中心が原点）
static MESH g_Sphere = {};		// 半径 1.0 の球（中心が原点）
static MESH g_Cylinder = {};	// 半径 1.0・高さ 1.0 の円柱の側面（中心が原点。ふたはない）


// 頂点とインデックスからバッファを作る（中身を変えないので DEFAULT で作り、作るときにデータを渡す）
static MESH CreateMesh(const VERTEX_3D* vertex, UINT vertexCount, const WORD* index, UINT indexCount)
{
	MESH mesh = {};
	mesh.IndexCount = indexCount;

	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * vertexCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertex;
	DirectXGetDevice()->CreateBuffer(&bd, &sd, &mesh.VertexBuffer);

	bd.ByteWidth = sizeof(WORD) * indexCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	sd.pSysMem = index;
	DirectXGetDevice()->CreateBuffer(&bd, &sd, &mesh.IndexBuffer);

	return mesh;
}

static void ReleaseMesh(MESH& mesh)
{
	SAFE_RELEASE(mesh.IndexBuffer);
	SAFE_RELEASE(mesh.VertexBuffer);
	mesh.IndexCount = 0;
}

// 格子状に並んだ頂点（rows 行 × cols 列）の、となり合う4つを三角形2枚にする
// 外から見て (r, c) が左上・(r, c+1) が右上・(r+1, c) が左下になるように頂点を並べておけば、時計回りになる
static void AddGridIndices(WORD* index, int rows, int cols)
{
	int n = 0;
	for (int r = 0; r < rows - 1; r++) {
		for (int c = 0; c < cols - 1; c++) {
			WORD a = (WORD)(r * cols + c);	// 左上
			WORD b = (WORD)(a + 1);			// 右上
			WORD d = (WORD)(a + cols);		// 左下
			WORD e = (WORD)(d + 1);			// 右下
			index[n++] = a; index[n++] = b; index[n++] = d;
			index[n++] = d; index[n++] = b; index[n++] = e;
		}
	}
}

static void CreateBox(void)
{
	static constexpr int FACE_COUNT = 6;
	// 各面の「法線」「外から見て右向き」「外から見て上向き」（作り方は stage.cpp と同じ）
	const XMFLOAT3 faces[FACE_COUNT][3] = {
		{ {  0.0f,  0.0f, -1.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, 1.0f,  0.0f } },	// 手前
		{ {  0.0f,  0.0f,  1.0f }, { -1.0f, 0.0f,  0.0f }, { 0.0f, 1.0f,  0.0f } },	// 奥
		{ {  1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f,  1.0f }, { 0.0f, 1.0f,  0.0f } },	// 右
		{ { -1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f,  0.0f } },	// 左
		{ {  0.0f,  1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, 0.0f,  1.0f } },	// 上
		{ {  0.0f, -1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, 0.0f, -1.0f } },	// 下
	};

	VERTEX_3D vertex[FACE_COUNT * 4];
	WORD index[FACE_COUNT * 6];
	for (int i = 0; i < FACE_COUNT; i++) {
		XMVECTOR r = XMLoadFloat3(&faces[i][1]) * 0.5f;
		XMVECTOR u = XMLoadFloat3(&faces[i][2]) * 0.5f;
		XMVECTOR c = XMLoadFloat3(&faces[i][0]) * 0.5f;	// 面の中心

		VERTEX_3D* v = &vertex[i * 4];
		XMStoreFloat3(&v[0].Position, c - r + u);	// 左上
		XMStoreFloat3(&v[1].Position, c + r + u);	// 右上
		XMStoreFloat3(&v[2].Position, c - r - u);	// 左下
		XMStoreFloat3(&v[3].Position, c + r - u);	// 右下
		for (int j = 0; j < 4; j++) {
			v[j].Normal = faces[i][0];
			v[j].Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
			v[j].TexCoord = { (float)(j % 2), (float)(j / 2) };
		}

		// 2行 × 2列の格子として三角形2枚にする
		AddGridIndices(&index[i * 6], 2, 2);
		for (int j = 0; j < 6; j++) {
			index[i * 6 + j] += (WORD)(i * 4);
		}
	}
	g_Box = CreateMesh(vertex, FACE_COUNT * 4, index, FACE_COUNT * 6);
}

// 球の表面の点（stack：上から何段目、slice：手前から右回りに何列目）。θ = 0 が手前（-Z）で、θ が増えると右（+X）へ回る
static XMFLOAT3 SpherePoint(int stack, int slice)
{
	const float phi = XM_PI * (float)stack / (float)SPHERE_STACKS;		// 北極（真上）からの角度
	const float theta = XM_2PI * (float)slice / (float)SPHERE_SLICES;
	return { sinf(phi) * sinf(theta), cosf(phi), -sinf(phi) * cosf(theta) };
}

static void CreateSphere(void)
{
	static constexpr int ROWS = SPHERE_STACKS + 1;
	static constexpr int COLS = SPHERE_SLICES + 1;	// 継ぎ目は同じ場所に2つ頂点を置く
	VERTEX_3D vertex[ROWS * COLS];
	WORD index[(ROWS - 1) * (COLS - 1) * 6];

	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLS; c++) {
			VERTEX_3D& v = vertex[r * COLS + c];
			v.Position = SpherePoint(r, c);
			v.Normal = v.Position;	// 半径 1 の球なので、位置がそのまま外向きの向き
			v.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
			v.TexCoord = { (float)c / (COLS - 1), (float)r / (ROWS - 1) };
		}
	}
	AddGridIndices(index, ROWS, COLS);
	g_Sphere = CreateMesh(vertex, ROWS * COLS, index, (ROWS - 1) * (COLS - 1) * 6);
}

static void CreateCylinder(void)
{
	static constexpr int ROWS = 2;	// 上の輪と下の輪
	static constexpr int COLS = SPHERE_SLICES + 1;
	VERTEX_3D vertex[ROWS * COLS];
	WORD index[(ROWS - 1) * (COLS - 1) * 6];

	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLS; c++) {
			// 赤道（stack = 半分）の点を使うと、球と同じ向きに並ぶ
			XMFLOAT3 p = SpherePoint(SPHERE_STACKS / 2, c);
			VERTEX_3D& v = vertex[r * COLS + c];
			v.Position = { p.x, r == 0 ? 0.5f : -0.5f, p.z };
			v.Normal = { p.x, 0.0f, p.z };
			v.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
			v.TexCoord = { (float)c / (COLS - 1), (float)r };
		}
	}
	AddGridIndices(index, ROWS, COLS);
	g_Cylinder = CreateMesh(vertex, ROWS * COLS, index, (ROWS - 1) * (COLS - 1) * 6);
}

// 形を、ワールド行列と色を指定して描く
static void DrawMesh(const MESH& mesh, const XMMATRIX& world, const XMFLOAT4& color)
{
	Shader_SetWorldMatrix(world);

	MATERIAL material{};
	material.Diffuse = color;
	material.Ambient = color;
	Shader_SetMaterial(material);

	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	DirectXGetDeviceContext()->IASetVertexBuffers(0, 1, &mesh.VertexBuffer, &stride, &offset);
	DirectXGetDeviceContext()->IASetIndexBuffer(mesh.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	DirectXGetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DirectXGetDeviceContext()->DrawIndexed(mesh.IndexCount, 0, 0);
}


void ShapeInitialize(void)
{
	CreateBox();
	CreateSphere();
	CreateCylinder();
}

void ShapeFinalize(void)
{
	ReleaseMesh(g_Cylinder);
	ReleaseMesh(g_Sphere);
	ReleaseMesh(g_Box);
}

void ShapeDrawBox(const XMFLOAT3& center, const XMFLOAT3& size, const XMFLOAT4& color)
{
	DrawMesh(g_Box, XMMatrixScaling(size.x, size.y, size.z) * XMMatrixTranslation(center.x, center.y, center.z), color);
}

void ShapeDrawSphere(const XMFLOAT3& center, float radius, const XMFLOAT4& color)
{
	DrawMesh(g_Sphere, XMMatrixScaling(radius, radius, radius) * XMMatrixTranslation(center.x, center.y, center.z), color);
}

void ShapeDrawCylinder(const XMFLOAT3& start, const XMFLOAT3& end, float radius, const XMFLOAT4& color)
{
	const VEC3 from{ start.x, start.y, start.z }, to{ end.x, end.y, end.z };
	const float length = Vec3Length(Vec3Sub(to, from));
	if (length <= 0.0f || radius <= 0.0f) return;
	const TENTACLE_PIECE_AXES axes = TentacleCalcPieceAxes(from, to, { 0, -1, 0 });
	const VEC3 x = Vec3Scale(axes.X, radius), y = Vec3Scale(axes.Y, length), z = Vec3Scale(axes.Z, radius);
	const VEC3 center = Vec3Scale(Vec3Add(from, to), 0.5f);
	const XMMATRIX world = XMMatrixSet(x.X, x.Y, x.Z, 0, y.X, y.Y, y.Z, 0,
		z.X, z.Y, z.Z, 0, center.X, center.Y, center.Z, 1);
	DrawMesh(g_Cylinder, world, color);
}

void ShapeDrawCapsule(const XMFLOAT3& bottom, float radius, float height, const XMFLOAT4& color)
{
	// 真ん中の円柱の長さ（全体の高さから、上下の半球の分を引く）
	float bodyLength = height - radius * 2.0f;
	if (bodyLength < 0.0f) {
		bodyLength = 0.0f;
	}
	const float lowerY = bottom.y + radius;			// 下の半球の中心
	const float upperY = lowerY + bodyLength;		// 上の半球の中心

	if (bodyLength > 0.0f) {
		DrawMesh(g_Cylinder,
			XMMatrixScaling(radius, bodyLength, radius) * XMMatrixTranslation(bottom.x, (lowerY + upperY) * 0.5f, bottom.z),
			color);
	}
	ShapeDrawSphere({ bottom.x, lowerY, bottom.z }, radius, color);
	ShapeDrawSphere({ bottom.x, upperY, bottom.z }, radius, color);
}
