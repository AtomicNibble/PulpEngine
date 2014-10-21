//--------------------------------------------------------------
//  Version: 1.0
//  MadeBy: WinCat
//  Site: tom-crowley.co.uk
//
//  Info: fixed Function Em
//--------------------------------------------------------------

#include "Vertexbase.inc"

struct VS_INPUT
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
};




struct VS_OUTPUT
{
    float4 ssPosition           	: SV_POSITION;    
    float4 color                	: COLOR;
    float2 texCoord             	: TEXCOORD;
};

struct PS_INPUT
{
    float4 ssPosition           	: SV_POSITION;
    float4 color                	: COLOR0;
    float2 texCoord             	: TEXCOORD0;
};

struct PS_OUTPUT
{
    float4 color                : SV_TARGET0;
};


Texture2D  	baseMap : register(t0);
SamplerState  	baseMapSampler;



VS_OUTPUT BasicVSTest(VS_INPUT input)
{
    VS_OUTPUT output;
    float4 position    	= mul(float4(input.osPosition, 1.0), worldViewProjectionMatrix);
    
    output.ssPosition    = position;
    output.texCoord      = input.texCoord;
    output.color           = input.color;
    return output;
}

PS_OUTPUT SolidWorldPS(PS_INPUT input)
{
    PS_OUTPUT output;
    float4 textureCol = baseMap.Sample(baseMapSampler, input.texCoord);
    output.color = input.color * textureCol;
    return output;
}


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

PS_OUTPUT FontPS(PS_INPUT input)
{
    PS_OUTPUT output;
    output.color = input.color;
    output.color.a *= baseMap.Sample(baseMapSampler, input.texCoord).a;
//  output.color.a = 1;
//  output.color = float4(1,1,1,1);
    return output;
}


