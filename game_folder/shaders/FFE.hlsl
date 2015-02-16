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
#ifdef IN_Color
    float4 color                	: COLOR0;
#endf // !IN_Color
};

struct VS_OUTPUT
{
    float4 ssPosition           	: SV_POSITION;    
    float2 texCoord             	: TEXCOORD;
#ifdef IN_Color
    float4 color                	: COLOR;
#endif // !IN_Color
};

struct PS_INPUT
{
    float4 ssPosition           	: SV_POSITION;
    float2 texCoord             	: TEXCOORD0;
#ifdef IN_Color
    float4 color                	: COLOR0;
#endif / !IN_Color
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
#ifdef IN_Color    
    output.color             	= input.color;
#endif // !IN_Color
    return output;
}

PS_OUTPUT SolidPS(PS_INPUT input)
{
    PS_OUTPUT output;
#ifdef IN_Color    
    output.color = input.color;
#else
    output.color = float4(1.0);
#endif // !IN_Color
    return output;
}

PS_OUTPUT TexturePS(PS_INPUT input)
{
    PS_OUTPUT output;
    float4 textureCol = baseMap.Sample(baseMapSampler, input.texCoord);
#ifdef IN_Color
    output.color = input.color * textureCol;
#else
    output.color = textureCol;
#endif // !IN_Color
    return output;
}



