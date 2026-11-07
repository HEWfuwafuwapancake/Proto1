// 頂点シェーダーとピクセルシェーダーで共通の定義
// ※ 定数バッファの番号と並びは shader.h / shader.cpp と合わせること

// 定数バッファ
cbuffer WorldBuffer : register(b0)
{
	matrix World;
}

cbuffer ViewBuffer : register(b1)
{
	matrix View;
}

cbuffer ProjectionBuffer : register(b2)
{
	matrix Projection;
}

struct MATERIAL
{
	float4 Diffuse;
	float4 Ambient;
	bool   TextureEnable;
	float3 Dummy;
};

cbuffer MaterialBuffer : register(b3)
{
	MATERIAL Material;
}

struct LIGHT
{
	bool   Enable;
	float3 Dummy;
	float4 Direction;
	float4 Diffuse;
	float4 Ambient;
};

cbuffer LightBuffer : register(b4)
{
	LIGHT Light;
}

cbuffer SkinnedBoneBuffer : register(b5)
{
	matrix Bones[64];
}

// 頂点シェーダーの入力（VERTEX_3D と同じ並び）
struct VS_IN
{
	float4 Position : POSITION0;
	float3 Normal   : NORMAL0;
	float4 Diffuse  : COLOR0;
	float2 TexCoord : TEXCOORD0;
};

// 頂点シェーダーの出力 = ピクセルシェーダーの入力
struct PS_IN
{
	float4 Position : SV_POSITION;
	float4 Diffuse  : COLOR0;
	float2 TexCoord : TEXCOORD0;
};

// ふつうの頂点と、骨で変形した頂点の両方を同じライティングで描く。
PS_IN TransformAndLight(VS_IN input)
{
	PS_IN output;
	matrix wvp = mul(mul(World, View), Projection);
	output.Position = mul(input.Position, wvp);
	output.TexCoord = input.TexCoord;

	if (Light.Enable)
	{
		float3 normal = normalize(mul(input.Normal, (float3x3)World));
		float light = saturate(dot(normal, -normalize(Light.Direction.xyz)));
		output.Diffuse.rgb = input.Diffuse.rgb * Material.Diffuse.rgb * Light.Diffuse.rgb * light
		                   + input.Diffuse.rgb * Material.Ambient.rgb * Light.Ambient.rgb;
		output.Diffuse.a = input.Diffuse.a * Material.Diffuse.a;
	}
	else
	{
		output.Diffuse = input.Diffuse * Material.Diffuse;
	}
	return output;
}
