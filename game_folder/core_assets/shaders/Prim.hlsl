
#include "Vertexbase.inc"

/*
cbuffer ObjectConstants  : register(b0)
{
	float4x4 worldToScreenMatrix;
	float4x4 worldViewProjectionMatrix;
};
*/

struct VS_INPUT
{
    float3 osPosition           	: POSITION;
    float2 tex 			: TEXCOORD0; // not used, but added so vertex layout can be shared.
    float4 color                	: COLOR0;
};

struct VS_OUTPUT
{
    float4 ssPosition           	: SV_POSITION;
    float4 color                	: COLOR;
};

struct PS_INPUT
{
    float4 ssPosition           	: SV_POSITION;
    float4 color                	: COLOR0;
};

struct PS_OUTPUT
{
    float4 color                : SV_TARGET0;
};



VS_OUTPUT PrimVS( VS_INPUT IN )
{
  VS_OUTPUT OUT;
  OUT.ssPosition = mul( worldToScreenMatrix, float4(IN.osPosition, 1.0) );
  OUT.color =  IN.color;
  return OUT;
}

PS_OUTPUT PrimPS( PS_INPUT IN )
{
	PS_OUTPUT output;
	output.color = IN.color;
	return output;
}