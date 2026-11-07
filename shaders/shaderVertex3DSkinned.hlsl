#include "shaderCommon.hlsli"

struct SKINNED_IN
{
	float3 Position : POSITION0;
	float3 Normal : NORMAL0;
	uint4 BoneIndices : BLENDINDICES0;
	float4 Weights : BLENDWEIGHT0;
};

PS_IN main(SKINNED_IN input)
{
	VS_IN vertex;
	vertex.Position = 0.0f;
	vertex.Normal = 0.0f;
	vertex.Diffuse = 1.0f;
	vertex.TexCoord = 0.0f;

	// 4本の骨で位置と法線を変形し、重みで足し合わせる。
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		matrix bone = Bones[input.BoneIndices[i]];
		vertex.Position += mul(float4(input.Position, 1.0f), bone) * input.Weights[i];
		vertex.Normal += mul(input.Normal, (float3x3)bone) * input.Weights[i];
	}
	return TransformAndLight(vertex);
}
