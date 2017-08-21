#include "Vertexbase.inc"

struct VS_INPUT
{
  float3 osPosition             : POSITION;
  float2 tex               : TEXCOORD0;
#ifdef IL_UV2
  float2 tex1              : TEXCOORD1;
#endif

  float4 color                  : COLOR0;
  float4 normal                 : NORMAL0;

#if X_HWSKIN
  int4 boneIndexes : BLENDINDICES0;
  float4 weights : BLENDWEIGHT0;
#endif
};

struct VS_OUTPUT
{
    float4 ssPosition               : SV_POSITION;
    float2 texCoord               : TEXCOORD0;
    float4 color                    : COLOR;
};

struct PS_OUTPUT
{
    float4 color                : SV_TARGET0;
};

Texture2D          diffuse : register(t0);
SamplerState        diffuseSampler;

#if X_HWSKIN
StructuredBuffer<float4x4> BoneMatrices;
#endif // !X_HWSKIN

VS_OUTPUT vs_main( VS_INPUT IN )
{
  VS_OUTPUT OUT;

    float4 pos = float4(IN.osPosition, 1.0);

#if X_HWSKIN
    float4 v = { 0.0f, 0.0f, 0.0f, 1.0f };

    for(int i = 0; i < 4; i++)
    {
      //Multiply position by bone matrix
      v += IN.weights * mul(pos, BoneMatrices[IN.boneIndexes[i]]);
    }

    pos += v;

#endif // !X_HWSKIN

  float4 worldPosition = mul( pos, worldMatrix );
  OUT.ssPosition = mul( worldPosition, worldToScreenMatrix );
  OUT.texCoord = IN.tex;
  OUT.color =  IN.color;
  return OUT;
}

PS_OUTPUT ps_main( VS_OUTPUT IN )
{
    PS_OUTPUT output;
    float4 texCol = diffuse.Sample(diffuseSampler, IN.texCoord);
    output.color = IN.color *  texCol;
    return output;
}