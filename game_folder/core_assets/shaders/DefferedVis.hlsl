
#include "Vertexbase.inc"
#include "ReadDeferred.hlsl"
#include "Linear.hlsl"


float4 VisualizeAlbedoPS(PS_DeferredPass_Input input) : SV_Target0
{
	float3 albedo =  albedoTexture.Sample(albedoTextureSampler, input.texCoord).rgb;
	return float4( albedo, 1 );	
}


float4 VisualizeNormalsPS(PS_DeferredPass_Input input) : SV_Target0
{
	float3 vsNormal = GetNormal(input.texCoord);
	float3 wsNormal = mul(vsNormal, (float3x3)cameraToWorldMatrix);
	return float4((wsNormal + 1) * 0.5, 1);
}

float4 VisualizeDepthPS(PS_DeferredPass_Input input) : SV_Target0
{
	return float4(depthTexture.Sample(depthTextureSampler, input.texCoord).rrr, 1);	
}





