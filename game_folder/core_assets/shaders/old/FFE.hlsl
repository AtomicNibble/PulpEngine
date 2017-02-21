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
    float4 color                	: COLOR0;
};

struct VS_OUTPUT
{
    float4 ssPosition           	: SV_POSITION;    
    float2 texCoord             	: TEXCOORD;
    float4 color                	: COLOR;
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


Texture2D  	baseMap : register(t0);
SamplerState  	baseMapSampler;


VS_OUTPUT BasicVS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.ssPosition        	= mul(float4(input.osPosition, 1.0), worldToScreenMatrix);
    output.texCoord          	= input.texCoord;
    output.color             	= input.color;
    return output;
}

PS_OUTPUT SolidPS(PS_INPUT input)
{
    PS_OUTPUT output;   
    output.color = input.color;
    return output;
}


PS_OUTPUT TexturePS(PS_INPUT input)
{
    PS_OUTPUT output;
    float4 textureCol = baseMap.Sample(baseMapSampler, input.texCoord);
    output.color = input.color * textureCol;
    return output;
}



