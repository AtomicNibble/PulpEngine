#include "Vertexbase.inc"
#include "Util.inc"

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


static float4 lightPos[2] = {
  float4(-128,136,72,1),
  float4(392,-160,72,1)
};

static float4 lightColor[2] = {
  float4(0.964706, 0.247059, 0.121569,1),
  float4(0,0,1,1)
};

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

  float4 worldPosition = mul( float4(pos, 1.0), worldMatrix );

#else
  float4 worldPosition = mul( src, worldMatrix );
#endif // !X_HWSKIN


  // Calculate the normal vector against the world matrix only.
  OUT.normal = mul(IN.normal.xyz, (float3x3)worldMatrix);
 // OUT.normal = mul(OUT.normal, (float3x3)worldToCameraMatrix);
  OUT.normal = normalize(OUT.normal);

  // vert in view space.
  OUT.viewPosition = mul(worldPosition, worldToCameraMatrix).xyz;
  OUT.viewPosition = worldPosition.xyz;

  OUT.ssPosition = mul( worldPosition, worldToScreenMatrix );
  OUT.texCoord = IN.tex;
  OUT.color = IN.color;
  return OUT;
}


float4 Diffuse(float4 radiance, float3 n, float3 l, float4 albedo)
{
  float d = saturate(dot(n, l));
  return d * albedo * radiance;
}

float4 ComputePointLight(float3 viewPosition, float3 vsNormal, float4 albedo,
  float4 lightColor, float3 lightPos, float invLightRadius)
{
    float3 l = lightPos - viewPosition;
    float d = length(l);
    l = l / d;

    float att = GetDistanceAttenuation(d, invLightRadius);

    float4 vsLightDirection = float4(-0.25,0.3,1,1);
    float innerCone = cos(radians(20.5));
    float outerCone = cos(radians(69.5));
    float epsilon   = innerCone - outerCone;
    float invConeDifference = 1.f / (innerCone - outerCone);

    float theta = dot(l, vsLightDirection.xyz);
    float intensity = saturate((theta - outerCone) * invConeDifference);

    att *= intensity;

    float4 radiance = lightColor * att;

    float4 col = Diffuse(radiance, vsNormal, l, albedo);
    return col;
}

float4 ComputeSpotLight(float3 viewPosition, float3 vsNormal, float4 albedo,
  float4 lightColor, float3 lightPos, float invLightRadius)
{
    float3 l = lightPos - viewPosition;
    float d = length(l);
    l = l / d;

    float att = GetDistanceAttenuation(d, invLightRadius);

    float4 radiance = lightColor * att;

    return Diffuse(radiance, vsNormal, l, albedo);
}

PS_OUTPUT ps_main( VS_OUTPUT IN )
{
    PS_OUTPUT output;
    float4 texCol = diffuse.Sample(diffuseSampler, IN.texCoord);

    float4 albedo = texCol * IN.color;
    float3 vsNormal = IN.normal;
    float3 viewPosition = IN.viewPosition;

    float4 lightColor1 = ComputePointLight(
      viewPosition, vsNormal, albedo, lightColor[0], lightPos[0].xyz, 1.f / 1024.f
    );

    float4 lightColor2 = ComputeSpotLight(
      viewPosition, vsNormal, albedo, lightColor[1], lightPos[1].xyz, 1.f / 128.f
    );

    output.color = saturate( lightColor1 + lightColor2);

    return output;
}