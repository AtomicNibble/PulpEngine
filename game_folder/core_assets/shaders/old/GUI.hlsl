//--------------------------------------------------------------
//  Version: 1.0
//  MadeBy: WinCat
//  Site: tom-crowley.co.uk
//
//  Info:  used for most GUI items.
//--------------------------------------------------------------

#include "Vertexbase.inc"
#include "Common.inc"

struct VS_INPUT
{
    float4 position       		: POSITION;
    float2 texCoord         	: TEXCOORD0;
    float4 color            		: COLOR0;
};

struct VS_OUTPUT
{
    float2 texCoord              	: TEXCOORD0;
    float4 color                 	: COLOR0;
    float4 position	        	: SV_POSITION;
};

struct PS_INPUT
{
    float2 texCoord                : TEXCOORD0;
    float4 color                  	: COLOR0;
};

struct Material
{
    float4	resultColor;
};


Texture2D  	baseMap : register(t0);
SamplerState  	baseMapSampler  : register(s0);


void SampleTexture( float2 texCoord, float4 color, inout float4 resultColor)
{
	resultColor = baseMap.Sample(baseMapSampler, texCoord) * color;
}


// =========== Vertex Shader ===========
VS_OUTPUT guiVS(VS_INPUT input)
{
    VS_OUTPUT output;

    // screen space input.
    output.position   	= input.position;
    output.color       	= input.color;
    output.texCoord     	= input.texCoord;

    return output;
}


// =========== Pixel Shader ===========
float4 guiPS(PS_INPUT input) : SV_TARGET
{
    float4	resultColor;

#if X_TEXTURED
    SampleTexture(input.texCoord, input.color, resultColor);
#else
   resultColor =  input.color;
#endif

    return resultColor;
}


float4 AlphaTestPS(PS_INPUT input) : SV_TARGET
{
    float4 resultColor;

#if X_TEXTURED
    SampleTexture(input.texCoord, input.color, resultColor);
#else
   resultColor =  input.color;
#endif

    clip(resultColor.a - 0.5);
    return resultColor;
}







