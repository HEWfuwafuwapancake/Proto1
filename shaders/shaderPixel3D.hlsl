#include "shaderCommon.hlsli"


Texture2D tex : register(t0);
SamplerState samplerState : register(s0);

//ピクセルシェーダ
float4 main(PS_IN input) : SV_TARGET
{
	float4 color = input.Diffuse;

	// テクスチャを使うときだけ、テクスチャの色を掛ける
	if (Material.TextureEnable)
	{
		color *= tex.Sample(samplerState, input.TexCoord);
	}

	// マップの景観を描くときだけ霧を掛ける（霧がなければそのまま）
	color.rgb = ApplyFog(color.rgb, input.WorldPosition);

	return color;
}
