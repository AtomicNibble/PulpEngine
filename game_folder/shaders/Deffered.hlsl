
#include "Vertexbase.inc"
#include "Linear.hlsl"




float3 PackNormal(float3 normal)
{
	return normal * 0.5 + 0.5;
}

float3 UnpackNormal(float3 normal)
{
	return normal * 2 - 1;
}

float4 ConvertNormal(float3 vsNormal)
{
	return float4( PackNormal(vsNormal), 0);
}


struct VS_WriteDeferred_Input
{
	float3 osPosition           	: POSITION;
	float2 texCoord             	: TEXCOORD0;
	float2 texCoord1             	: TEXCOORD1;
	float4 color                		: COLOR0;
	float3 osNormal           		: NORMAL0;  	
};


struct VS_WriteDeferred_Output
{ 
	float4 depth                   	: TEXCOORD0;
	float2 texCoord             	: TEXCOORD1;  
	float3 vsNormal                : TEXCOORD2;
	float4 color                		: COLOR0;
	
	// not passed
	float4 ssPosition           	: SV_POSITION;   
};

struct PS_WriteDeferred_Input
{
	float4 depth                   	: TEXCOORD0;
	float2 texCoord             	: TEXCOORD1;  
	float3 vsNormal                : TEXCOORD2;
	float4 color                		: COLOR0;
};


struct PS_WriteDeferred_Output
{
	float4 albedo                   : SV_Target0;
	float4 normal                   : SV_Target1;
	float4 depth                    : SV_Target2;
};

Texture2D  		baseMap : register(t0);
SamplerState  	baseMapSampler;


VS_WriteDeferred_Output WriteDeferredVS(VS_WriteDeferred_Input input)
{
	VS_WriteDeferred_Output output;
	
	float4x4 objectToWorldMatrixInvTrans = objectToWorldMatrix;

	// word space
	float4 wsPosition  = mul(float4(input.osPosition, 1.0), objectToWorldMatrix);
	float3 wsNormal   = normalize( mul(input.osNormal,   (float3x3)objectToWorldMatrixInvTrans ) );

	// View Space
	float3 vsNormal   	= mul(wsNormal, (float3x3)worldToCameraMatrix );


	float vsDepth = wsPosition.x * worldToCameraMatrix[0][2] +
                    wsPosition.y * worldToCameraMatrix[1][2] +
                    wsPosition.z * worldToCameraMatrix[2][2] +  
										worldToCameraMatrix[3][2];

	
	output.depth          		= float4( vsDepth, 0, 0, 0 );	 
	output.texCoord      	= input.texCoord;
    output.vsNormal     		= vsNormal;
	output.color            	= input.color;
	
	// vertex only
	output.ssPosition     	= mul(wsPosition, worldToScreenMatrix);

	return output;
}


PS_WriteDeferred_Output DeferredShadingPS(PS_WriteDeferred_Input input)
{
	PS_WriteDeferred_Output output;
	
	float4 textureCol = baseMap.Sample(baseMapSampler, input.texCoord);
 
     // Transform the normal into view space.
    float3 vsNormal = normalize(input.vsNormal);  
 
	output.albedo 			=  input.color * textureCol; 
	output.normal          = ConvertNormal(vsNormal);
	output.depth           = float4(input.depth.r, input.depth.g, 0, 0);
	return output;
}


