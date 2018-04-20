
#include "Vertexbase.inc"
#include "Util.inc"

struct VS_INPUT
{
    float3 osPosition           	: POSITION0;
    float2 tex 			: TEXCOORD0;
    float4 color                	: COLOR0;

#if X_INSTANCED
    float4 instPos1             : POSITION1;
    float4 instPos2            : POSITION2;
    float4 instPos3             : POSITION3;
    float4 instPos4             : POSITION4;
    float4 instColor           : COLOR1;

    // uint instanceID : SV_InstanceID;
#endif // !X_INSTANCED
};

struct VS_OUTPUT
{
    float4 ssPosition           	: SV_POSITION;
    float4 color                	: COLOR;
#if X_TEXTURED
    float2 texCoord                 : TEXCOORD0;
#endif // !X_TEXTURED
};

struct PS_INPUT
{
    float4 ssPosition           	: SV_POSITION;
    float4 color                	: COLOR0;
#if X_TEXTURED
    float2 texCoord                 : TEXCOORD0;
#endif // !X_TEXTURED
};

struct PS_OUTPUT
{
    float4 color                    : SV_TARGET0;
};

#if X_TEXTURED
Texture2D           colorMap : register(t0);
SamplerState        colorSampler;

#endif // !X_TEXTURED

VS_OUTPUT PrimVS( VS_INPUT IN )
{
  VS_OUTPUT OUT;

#if X_INSTANCED
    float4x4 instPos = CreateMatrixFromCols(IN.instPos1, IN.instPos2, IN.instPos3, IN.instPos4);
    float4x4 mat = mul(worldToScreenMatrix, instPos);
    OUT.ssPosition = mul(mat, float4(IN.osPosition, 1.0));
    OUT.color =  IN.color * IN.instColor;
#else
    OUT.ssPosition = mul(worldToScreenMatrix, float4(IN.osPosition, 1.0));
    OUT.color =  IN.color;
#endif // !X_INSTANCED

#if X_TEXTURED
    OUT.texCoord = IN.tex;
#endif // !X_TEXTURED

  return OUT;
}

PS_OUTPUT PrimPS( PS_INPUT IN )
{
	PS_OUTPUT output;
	output.color = IN.color;

#if X_TEXTURED
    float4 texCol = colorMap.Sample(colorSampler, IN.texCoord);
    output.color = output.color *  texCol;
#endif // !X_TEXTURED

	return output;
}