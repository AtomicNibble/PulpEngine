

cbuffer ObjectConstants  : register(b0)
{
	float4x4 worldToScreenMatrix;
	float4x4 worldViewProjectionMatrix;

	float4 auxGeomObjColor;
	float2 auxGeomObjShading;
};


struct VS_INPUT
{
    float3 osPosition           	: POSITION;
    float2 tex 			: TEXCOORD0; // not used, but added so vertex layout can be shared.
    float4 color                	: COLOR0;
};

struct VS_INPUT_OBJ
{
    float3 osPosition           	: POSITION;
    float3 normal 			: TEXCOORD0;
};



struct VS_OUTPUT
{
    float4 ssPosition           	: SV_POSITION;
    float4 color                	: COLOR;
};

struct PS_INPUT
{
    float4 ssPosition           	: SV_POSITION;
    float4 color                	: COLOR0;
};

struct PS_OUTPUT
{
    float4 color                : SV_TARGET0;
};



VS_OUTPUT AuxGeomVS( VS_INPUT IN )
{
  VS_OUTPUT OUT;
  OUT.ssPosition = mul( worldToScreenMatrix, float4(IN.osPosition, 1.0) );
  OUT.color =  IN.color * auxGeomObjColor;
  return OUT;
}

VS_OUTPUT AuxGeomObjVS( VS_INPUT_OBJ IN )
{
  VS_OUTPUT OUT;
  OUT.ssPosition = mul( worldViewProjectionMatrix, float4(IN.osPosition, 1.0) );

  float shading = auxGeomObjShading.x * auxGeomObjShading.y;
  OUT.color = float4( auxGeomObjColor.rgb, auxGeomObjColor.a );

  return OUT;
}

PS_OUTPUT AuxGeomPS( PS_INPUT IN )
{
	PS_OUTPUT output;
	output.color = IN.color;
	return output;
}