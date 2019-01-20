#include "Vertexbase.inc"
#include "Util.inc"
#include "Lighting.inc"

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
    float2 texCoord                 : TEXCOORD0;
    float4 color                    : COLOR;

    // Lights
    float3 normal : NORMAL;
    float3 viewPosition : TEXCOORD1;
};

struct PS_OUTPUT
{
    float4 color                : SV_TARGET0;
};

Texture2D           diffuse : register(t0);
SamplerState        diffuseSampler;

#if X_HWSKIN
StructuredBuffer<float4x4> BoneMatrices;
#endif // !X_HWSKIN

#if 1
StructuredBuffer<Light> lights;
#else
static Light lights[2] = {
    {
      float4(-200,275,72,1),
      float4(0.964706, 0.247059, 0.121569,1),
      float4(-0.05,0.5,1,1),
      cos(radians(50.0)),
      cos(radians(75.0)),
      (1.f / (cos(radians(50.0)) - cos(radians(75.0)))),
      1.f / 1024.f
    },
    {
      float4(0,136,72,1),
      float4(0,1,0,1),
      float4(0,0,0,0),
      cos(radians(20.5)),
      cos(radians(69.5)),
      (1.f / (cos(radians(20.5)) - cos(radians(69.5)))),
      1.f / 128.f
    }
};
#endif

VS_OUTPUT vs_main( VS_INPUT IN )
{
  VS_OUTPUT OUT;

  float4 src = float4(IN.osPosition, 1.0f);

#if X_HWSKIN
    float3 pos = { 0.0f, 0.0f, 0.0f };

    for(int i = 0; i < 4; i++)
    {
      // pos +=  (mul(src, BoneMatrices[IN.boneIndexes[i]]).xyz * IN.weights[i]);
      pos +=  (mul(BoneMatrices[IN.boneIndexes[i]], src).xyz * IN.weights[i]);
    }

  float4 worldPosition = mul( worldMatrix, float4(pos, 1.0) );

#else
  float4 worldPosition = mul( worldMatrix, src );
#endif // !X_HWSKIN


  // Calculate the normal vector against the world matrix only.
  OUT.normal = mul((float3x3)worldMatrix, IN.normal.xyz);
 // OUT.normal = mul((float3x3)worldToCameraMatrix, OUT.normal);
  OUT.normal = normalize(OUT.normal);

  // vert in view space.
  OUT.viewPosition = mul(worldToCameraMatrix, worldPosition).xyz;
  OUT.viewPosition = worldPosition.xyz;

  OUT.ssPosition = mul(worldToScreenMatrix,  worldPosition);
  OUT.texCoord = IN.tex;
  OUT.color = IN.color;
  return OUT;
}


PS_OUTPUT ps_main( VS_OUTPUT IN )
{
    PS_OUTPUT output;
    float4 texCol = diffuse.Sample(diffuseSampler, IN.texCoord);

    float4 albedo = texCol * IN.color;
    float3 vsNormal = IN.normal;
    float3 viewPosition = IN.viewPosition;

    float4 ambient = albedo * float4(0.5,0.5,0.5,1);

    float4 lightColor1 = ComputePointLight(
      viewPosition, vsNormal, albedo, lights[0]
    );


    float4 lightColor2 = ComputePointLight(
      viewPosition, vsNormal, albedo, lights[1]
    );

    output.color = saturate(ambient + lightColor1 + lightColor2);

    return output;
}