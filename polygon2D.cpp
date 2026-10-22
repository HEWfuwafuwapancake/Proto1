#include "main.h"
#include "dx11.h"
#include "shader.h"
#include "polygon2D.h"


static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ

void Polygon2DInitialize(void)
{
	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4;//頂点数4
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DirectXGetDevice()->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	DirectXGetDeviceContext()->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	VERTEX_3D* v = (VERTEX_3D*)msr.pData;

	// 頂点座標（画面の左上が原点、ピクセル単位）
	float x = 20.0f;
	float y = 20.0f;
	float width = 300.0f;
	float height = 60.0f;
	v[0].Position = { (float)x,         (float)y,          0.0f };
	v[1].Position = { (float)x + width, (float)y,          0.0f };
	v[2].Position = { (float)x,         (float)y + height, 0.0f };
	v[3].Position = { (float)x + width, (float)y + height, 0.0f };

	//テクスチャ座標
	v[0].TexCoord = { 0.0f, 0.0f };
	v[1].TexCoord = { 1.0f, 0.0f };
	v[2].TexCoord = { 0.0f, 1.0f };
	v[3].TexCoord = { 1.0f, 1.0f };

	//法線と頂点カラー（半透明の黒。2Dはライトを使わないので法線は使われない）
	for (int i = 0; i < 4; i++) {
		v[i].Normal = { 0.0f, 0.0f, -1.0f };
		v[i].Diffuse = { 0.0f, 0.0f, 0.0f, 0.5f };
	}

	// 頂点バッファのロックを解除
	DirectXGetDeviceContext()->Unmap(g_pVertexBuffer, 0);

}

void Polygon2DFinalize(void)
{
	SAFE_RELEASE(g_pVertexBuffer);
}

void Polygon2DUpdate(void)
{

}

void Polygon2DDraw(void)
{
	//2Dマトリクス設定
	Shader_Set2DMatrix();

	// マテリアル（頂点カラーをそのまま使う）
	MATERIAL material{};
	material.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	material.Ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
	Shader_SetMaterial(material);

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	DirectXGetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// プリミティブトポロジの設定
	DirectXGetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// ポリゴン描画命令発行
	DirectXGetDeviceContext()->Draw(4, 0);//頂点数4
}
