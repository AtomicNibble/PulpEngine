
#include "Vertexbase.inc"
#include "Util.inc"

struct VS_INPUT
{
    float3 osPosition           	: POSITION0;
    float2 tex 			: TEXCOORD0; // not used, but added so vertex layout can be shared.
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


VS_OUTPUT PrimVS( VS_INPUT IN )
{
  VS_OUTPUT OUT;

#if X_INSTANCED
    float4x4 instPos = CreateMatrixFromRows(IN.instPos1, IN.instPos2, IN.instPos3, IN.instPos4);
    float4x4 mat = mul(instPos, worldToScreenMatrix);
    OUT.ssPosition = mul( float4(IN.osPosition, 1.0), mat );
    OUT.color =  IN.color  * IN.instColor;
#else
    OUT.ssPosition = mul( float4(IN.osPosition, 1.0), worldToScreenMatrix );
    OUT.color =  IN.color;
#endif // !X_INSTANCED
  return OUT;
}

PS_OUTPUT PrimPS( PS_INPUT IN )
{
	PS_OUTPUT output;
	output.color = IN.color;
	return output;
}