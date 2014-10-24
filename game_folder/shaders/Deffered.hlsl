
#include "Vertexbase.inc"
#include "Linear.hlsl"

cbuffer ObjectConstants  : register(cb1)
{
    float4x4    objectToWorldMatrix;
};



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
#ifdef PARAM_VS_Normal
    float3 osNormal           	: NORMAL0;  
#endif
    float4 color                	: COLOR0;
    float2 texCoord             	: TEXCOORD0;
#ifdef PARAM_VS_Texcoord2
    float2 texCoord2             	: TEXCOORD1;
#endif
    
    
    /*
    float3 osPosition               : POSITION;
    float4 color                    : COLOR;
    float2 texCoord                 : TEXCOORD0;
    float3 osNormal                 : NORMAL;
    float3 osBinormal               : BINORMAL;
    float3 osTangent                : TANGENT;
*/
};


struct VS_WriteDeferred_Output
{
    float4 ssPosition           	: SV_POSITION;    
    float4 color                	: COLOR0;
    float2 texCoord             	: TEXCOORD0;
};

struct PS_WriteDeferred_Input
{
    float4 ssPosition           	: SV_POSITION;
    float4 color                	: COLOR0;
    float2 texCoord             	: TEXCOORD0;
};


struct PS_WriteDeferred_Output
{
    float4 albedo                   : SV_Target0;
    float4 normal                   : SV_Target1;
    float4 depth                    : SV_Target2;
};

Texture2D  	baseMap : register(t0);
SamplerState  	baseMapSampler;


VS_WriteDeferred_Output WriteDeferredVS(VS_WriteDeferred_Input input)
{
   VS_WriteDeferred_Output output;
   
    float4 position    	= mul(float4(input.osPosition, 1.0), worldViewProjectionMatrix);
  
  
    output.ssPosition     = position;
    output.texCoord      = input.texCoord;
    output.color            = input.color;
    return output;
}


PS_WriteDeferred_Output DeferredShadingPS(PS_WriteDeferred_Input input)
{
    PS_WriteDeferred_Output output;
    
    float4 textureCol = baseMap.Sample(baseMapSampler, input.texCoord);
 
    output.albedo 		=  input.color * textureCol; 
    output.normal          = float4(1,1,0,1);
    output.depth           = float4(0.9, 0, 0, 1);
    return output;
}




