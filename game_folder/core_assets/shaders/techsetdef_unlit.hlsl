#include "Vertexbase.inc"
#include "Util.inc"
#include "Lighting.inc"

struct VS_INPUT
{
  float3 osPosition                 : POSITION;
  float2 tex                        : TEXCOORD0;
#ifdef IL_UV2
  float2 tex1                       : TEXCOORD1;
#endif

  float4 color                      : COLOR0;
  float4 normal                     : NORMAL0;

#if IL_BINORMAL
  float4 tangent                    : TANGENT0;
  float4 binormal                   : BINORMAL0;
#endif

#if X_HWSKIN
  int4 boneIndexes                  : BLENDINDICES0;
  float4 weights                    : BLENDWEIGHT0;
#endif
};

struct VS_OUTPUT
{
    float4 ssPosition               : SV_POSITION;
    float3 viewPosition             : TEXCOORD1;
    float3 worldPosition            : TEXCOORD2;
    float2 texCoord                 : TEXCOORD0;
    float4 color                    : COLOR;

    float3 normal                   : NORMAL;
#if IL_BINORMAL
    float3 tangent                  : TANGENT0;
    float3 binormal                 : BINORMAL0;
#endif
};

struct PS_OUTPUT
{
    float4 color                    : SV_TARGET0;
};

struct Attributes
{
  float3 position;
  float2 uv;
  float3 normal;
  float3 binormal;
  float3 tangent;
};

struct Material
{
  float4 albedo;
  float roughness;
  float ao;
  float metalness;
};

Texture2D           diffuse : register(t0);
SamplerState        diffuseSampler;
Texture2D           normalMap;
SamplerState        normalSampler;

Texture2D           roughnessMap;
Texture2D           ambientOcclusionMap;
Texture2D           metallicMap;
//Texture2D           displacementMap;


float4 GammaCorrectTexture(Texture2D t, SamplerState s, float2 uv)
{
  float4 samp = t.Sample(s, uv);
  return float4(pow(abs(samp.rgb), GAMMA), samp.a);
}

float3 GammaCorrectTextureRGB(Texture2D t, SamplerState s, float2 uv)
{
  float3 samp = t.Sample(s, uv).rgb;
  return float3(pow(abs(samp), GAMMA));
}

float4 GetAlbedo(Attributes attributes)
{
  return GammaCorrectTexture(diffuse, diffuseSampler, attributes.uv);
}

float GetAO(Attributes attributes)
{
  return GammaCorrectTextureRGB(ambientOcclusionMap, normalSampler, attributes.uv).r;
}

float GetRoughness(Attributes attributes)
{
  return GammaCorrectTextureRGB(roughnessMap, normalSampler, attributes.uv).r;
}

float GetMetallic(Attributes attributes)
{
  return GammaCorrectTextureRGB(metallicMap, normalSampler, attributes.uv).r;
}

float3 FinalGamma(float3 color)
{
  return pow(abs(color), 1.0f / GAMMA);
}

float3 NormalMap(Attributes attributes)
{
  float3 normal = normalMap.Sample(normalSampler, attributes.uv).rgb;
  normal.g = 1.0 - normal.g;
  normal = normal * 2.0 - 1.0;

#if IL_BINORMAL
  float3x3 toWorld = float3x3(attributes.binormal, attributes.tangent, attributes.normal);
  normal = mul(normal.rgb, toWorld);
#else
  //float3x3 toWorld = float3x3(attributes.normal, attributes.normal, attributes.normal);
  //normal = mul(normal.rgb, toWorld);
#endif

  normal = normalize(normal);
  return normal;
}


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


static const float3 lightVector = float3(0.6,0.6,0.6);

VS_OUTPUT vs_main( VS_INPUT IN )
{
  VS_OUTPUT OUT;

  float4 src = float4(IN.osPosition, 1.0f);

#if X_HWSKIN
  float3 pos = { 0.0f, 0.0f, 0.0f };

  for(int i = 0; i < 4; i++)
  {
    pos +=  (mul(BoneMatrices[IN.boneIndexes[i]], src).xyz * IN.weights[i]);
  }

  float4 worldPosition = mul( worldMatrix, float4(pos, 1.0) );
#else
  float4 worldPosition = mul( worldMatrix, src );
#endif // !X_HWSKIN

  // Calculate the normal vector against the world matrix only.
  OUT.normal = normalize(mul((float3x3)worldMatrix, IN.normal.xyz));

#if IL_BINORMAL
  OUT.binormal = normalize(mul((float3x3)worldMatrix, IN.binormal.xyz));
  OUT.tangent = normalize(mul((float3x3)worldMatrix, IN.tangent.xyz));
#endif

  // vert in view space.
  OUT.worldPosition = worldPosition.xyz;
  OUT.viewPosition = mul(worldToCameraMatrix, worldPosition).xyz;
  OUT.ssPosition = mul(worldToScreenMatrix,  worldPosition);

  OUT.texCoord = IN.tex;
  OUT.color = IN.color;
  return OUT;
}

// Constant normal incidence Fresnel factor for all dielectrics.
static const float3 Fdielectric = 0.04;
static const float Epsilon = 0.00001;

PS_OUTPUT ps_main( VS_OUTPUT input )
{
  Attributes attributes;
  attributes.position = input.worldPosition;
  attributes.uv = input.texCoord;
  attributes.normal = normalize(input.normal);

#if IL_BINORMAL
  attributes.binormal = normalize(input.binormal);
  attributes.tangent = normalize(input.tangent);
#endif

  attributes.normal = NormalMap(attributes);

  Material material;
  material.albedo = GetAlbedo(attributes);
  material.roughness = GetRoughness(attributes);
  material.ao = GetAO(attributes);
  material.metalness = GetMetallic(attributes);

  // need like a global light for ambient?

// then we need to slap up all my nigerian homies, tell them what's up
// shit's about to go down.


  // Outgoing light direction (vector from world-space fragment position to the "eye").
  // Lo = light out
  // Li = light int
  float3 Lo = normalize(cameraPos.xyz - attributes.position);

  // Angle between surface normal and outgoing light direction.
  float cosLo = max(0.0, dot(attributes.normal, Lo));

  // Fresnel reflectance at normal incidence (for metals use albedo color).
  float3 F0 = lerp(Fdielectric, material.albedo.rgb, material.metalness);

  float3 directLighting = 0.0;
  for(int i=0; i<4; i++)
  {
    Light light = lights[i];

//    float3 Lradiance = float3(0.5, 0.5, 0.5); // lights[i].radiance;

    // Calculate the light vector.
    float3 Li = normalize(light.pos.xyz - attributes.position.xyz);
    // Half-vector between Li and Lo.
    float3 Lh = normalize(Li + Lo);

    // calculate per-light radiance
    //float3 L = normalize(light.pos.xyz - attributes.position.xyz);
    //float3 H = normalize(V + L);
    float dist        = length(light.pos.xyz - attributes.position.xyz) * 0.01;
    float attenuation = 1.0 / (dist * dist);
    //float3 Lradiance  = light.color.rgb * attenuation;
    float3 Lradiance  = float3(0.9,0.9,0.9) * attenuation;


    // Calculate angles between surface normal and various light vectors.
    float cosLi = max(0.0, dot(attributes.normal, Li));
    float cosLh = max(0.0, dot(attributes.normal, Lh));

    // Calculate Fresnel term for direct lighting.
    float3 F  = FresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
    // Calculate normal distribution for specular BRDF.
    float D = NormalDistributionGGX(cosLh, material.roughness);
    // Calculate geometric attenuation for specular BRDF.
    float G = GeometrySchlickGGX(cosLi, cosLo, material.roughness);


    // Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
    // Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
    // To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
    float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), material.metalness);

    // Lambert diffuse BRDF.
    // We don't scale by 1/PI for lighting & material units to be more convenient.
    // See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
    float3 diffuseBRDF = kd * material.albedo.rgb;

    // Cook-Torrance specular microfacet BRDF.
    float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);

    // Total contribution for this light.
    directLighting += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
  }

  const float amb = 0.03; // 0.03;
  float3 ambient = float3(amb, amb, amb) * material.albedo.rgb * material.ao;

  // TODO: Ambient lighting (IBL).
  float3 ambientLighting = ambient;

  float3 finalColor = directLighting + ambientLighting;
  finalColor = FinalGamma(finalColor);

  PS_OUTPUT output;
  output.color = float4(finalColor, 1.0);
  return output;
}