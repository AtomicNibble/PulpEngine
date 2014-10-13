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
    float4 color              	: COLOR0;  
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
SamplerState  	baseMapSampler;


void MaterialShader( float2 texCoord, float4 color, inout Material material)
{
	material.resultColor = baseMap.Sample(baseMapSampler, texCoord) * color;
}


// =========== Vertex Shader ===========
VS_OUTPUT guiVS(VS_INPUT input)
{
    VS_OUTPUT output;
    
    output.position   	= mul(input.position, worldToScreenMatrix);
    output.color       	= input.color * time;
    output.texCoord     	= input.texCoord;

    return output;
}

// =========== Pixel Shader ===========
float4 guiPS(PS_INPUT input) : SV_TARGET
{
    Material material;
    
#if X_TEXTURED
    MaterialShader(input.texCoord, input.color, material);
#else
   material.resultColor =  input.color;
#endif

    return material.resultColor;
}


float4 AlphaTestPS(PS_INPUT input) : SV_TARGET
{
    Material material;
    
#if X_TEXTURED
    MaterialShader(input.texCoord, input.color, material);
#else
   material.resultColor =  input.color;
#endif
 
    clip(material.resultColor.a - 0.5);
    return material.resultColor;
}







