//--------------------------------------------------------------
//  Version: 1.0
//  MadeBy: WinCat
//  Site: tom-crowley.co.uk
//
//  Info: fixed Function, used for drawing genral quads, used for debug drawing.
//--------------------------------------------------------------

#include "Vertexbase.inc"

struct VS_INPUT
{
	float3 osPosition           	: POSITION;
	float2 texCoord             	: TEXCOORD0;
	float2 texCoord1             	: TEXCOORD1;
	float4 color                	: COLOR0;
	float4 normal                	: NORMAL0;
};

struct VS_OUTPUT
{
	float4 ssPosition           	: SV_POSITION;    
	float2 texCoord             	: TEXCOORD;
	float4 color                	: COLOR;
	float4 normal			: NORMAL0;
};

struct PS_INPUT
{
	float4 ssPosition           	: SV_POSITION;
	float2 texCoord             	: TEXCOORD0;
	float4 color                	: COLOR0;
};

struct PS_OUTPUT
{
	float4 color                : SV_TARGET0;
};

Texture2D  		baseMap : register(t0);
SamplerState  	baseMapSampler;

Texture2D  		bumpMap : register(t1);
SamplerState  	bumpMapSampler;

VS_OUTPUT BasicVS(VS_INPUT input)
{
	VS_OUTPUT output;
	output.ssPosition        		= mul(float4(input.osPosition, 1.0), worldToScreenMatrix);
	output.texCoord          	= input.texCoord;
	output.color             		= input.color;
	output.normal 					= mul(input.normal,objectToWorldMatrix);
	return output;
}

PS_OUTPUT SolidPS(PS_INPUT input)
{
	PS_OUTPUT output;   
	float4 textureCol = baseMap.Sample(baseMapSampler, input.texCoord);
	float4 normalCol = bumpMap.Sample(bumpMapSampler, input.texCoord);
	 
	float4 bump;
	bump.xy	= (normalCol.ag * 2.0) - 1.0;
	bump.z = sqrt(1.0 - dot(bump.xy, bump.xy));
	bump.a = 1.0;
	 
//	bump = normalize(bump);
	 
	output.color = input.color * (textureCol * saturate(dot(bump, float4(0.7,0.9,0.1,1))));
// 	output.color = textureCol;
	return output;
}

