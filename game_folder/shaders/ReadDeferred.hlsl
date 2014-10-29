


struct VS_DeferredPass_Input
{
    float3 osPosition       : POSITION;
    float2 texCoord         : TEXCOORD0;
    float4 color              : COLOR0;  
};

struct VS_DeferredPass_Output
{
    float3 projected        : TEXCOORD0;
    float2 texCoord         : TEXCOORD1;
    float4 ssPosition       : SV_POSITION;
};

struct PS_DeferredPass_Input
{
    float3 projected        : TEXCOORD0;
    float2 texCoord         : TEXCOORD1;
};  



Texture2D   depthTexture           : register(t0);     // G-buffer component.
Texture2D	 normalTexture          : register(t1);     // G-buffer component.
Texture2D   albedoTexture          : register(t2);     // G-buffer component.

SamplerState  	depthTextureSampler;
SamplerState  	normalTextureSampler;
SamplerState  	albedoTextureSampler;


// Extracts the view-space normal from the G-buffer.
float3 GetNormal(float2 texCoord)
{
	float3 G = normalTexture.Sample(normalTextureSampler, texCoord).xyz;
	return G * 2.0 - 1.0;
}


VS_DeferredPass_Output DeferredPassVS(VS_DeferredPass_Input input)
{
	VS_DeferredPass_Output output;
    
	float4 ssPosition = float4(input.osPosition, 1);

	output.ssPosition   = ssPosition;
	output.projected.x  = ssPosition.x;
	output.projected.y  = ssPosition.y;
	output.projected.z  = 1;
	output.projected.xy *= -screensize.xy;
	
	output.texCoord     = input.texCoord;

	return output;
}