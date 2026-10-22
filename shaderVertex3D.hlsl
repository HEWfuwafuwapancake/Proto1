#include "shaderCommon.hlsli"


//頂点シェーダ
PS_IN main(VS_IN input)
{
	PS_IN output;

	// ワールド → ビュー → プロジェクションの順に変換して画面上の位置を求める
	matrix wvp = mul(mul(World, View), Projection);
	output.Position = mul(input.Position, wvp);
	output.TexCoord = input.TexCoord;

	if (Light.Enable)
	{
		// 法線をワールド空間に回して、光の向きとの角度で明るさを決める（ランバート）
		float3 normal = normalize(mul(input.Normal, (float3x3)World));
		float  light = saturate(dot(normal, -normalize(Light.Direction.xyz)));

		output.Diffuse.rgb = input.Diffuse.rgb * Material.Diffuse.rgb * Light.Diffuse.rgb * light
		                   + input.Diffuse.rgb * Material.Ambient.rgb * Light.Ambient.rgb;
		output.Diffuse.a = input.Diffuse.a * Material.Diffuse.a;
	}
	else
	{
		// ライトなし（2D など）：頂点カラー × マテリアルの色そのまま
		output.Diffuse = input.Diffuse * Material.Diffuse;
	}

	return output;
}
