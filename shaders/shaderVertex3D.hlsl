#include "shaderCommon.hlsli"

PS_IN main(VS_IN input)
{
	return TransformAndLight(input);
}
