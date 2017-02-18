
#include "Vertexbase.inc"

struct VS_INPUT
{
  float3 osPosition             : POSITION;
  float2 tex               : TEXCOORD0;
  float2 tex1              : TEXCOORD1;
  float4 color                  : COLOR0;
  float4 normal                 : NORMAL0;
};

struct VS_OUTPUT
{
    float4 ssPosition               : SV_POSITION;
    float2 texCoord               : TEXCOORD0;
    float4 color                    : COLOR;
};

struct PS_INPUT
{
    float4 ssPosition               : SV_POSITION;
    float2 texCoord               : TEXCOORD0;
    float4 color                    : COLOR0;
};

struct PS_OUTPUT
{
    float4 color                : SV_TARGET0;
};

Texture2D          diffuse : register(t0);
SamplerState        diffuseSampler;


VS_OUTPUT vs_main( VS_INPUT IN )
{
  VS_OUTPUT OUT;
  OUT.ssPosition = mul( float4(IN.osPosition, 1.0), worldToScreenMatrix );
  OUT.texCoord = IN.tex;
  OUT.color =  IN.color;
  return OUT;
}

PS_OUTPUT ps_main( PS_INPUT IN )
{
    PS_OUTPUT output;
    float4 texCol = diffuse.Sample(diffuseSampler, IN.texCoord);
    output.color = IN.color *  texCol;
    return output;
}