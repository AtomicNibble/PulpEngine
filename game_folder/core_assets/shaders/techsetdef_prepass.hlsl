
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
    float4 color                    : COLOR;
};

struct PS_INPUT
{
    float4 ssPosition               : SV_POSITION;
    float4 color                    : COLOR0;
};

struct PS_OUTPUT
{
    float4 color                : SV_TARGET0;
};



VS_OUTPUT vs_main( VS_INPUT IN )
{
  VS_OUTPUT OUT;
  OUT.ssPosition = mul( worldToScreenMatrix, float4(IN.osPosition, 1.0));
  OUT.color =  IN.color;
  return OUT;
}

PS_OUTPUT ps_main( PS_INPUT IN )
{
    PS_OUTPUT output;
    output.color = IN.color;
    output.color.r =  IN.ssPosition.x;
    output.color.g =  IN.ssPosition.y;
    output.color.b =  IN.ssPosition.z;
    return output;
}