
#include "Vertexbase.inc"
#include "Linear.hlsl"

cbuffer ObjectConstants  : register(cb1)
{
    float4x4    objectToWorldMatrix;
};


struct VS_WriteDeferred_Input
{
    float3 osPosition           	: POSITION;
    float4 color                	: COLOR0;
    float2 texCoord             	: TEXCOORD0;
    /*
    float3 osPosition               : POSITION;
    float4 color                    : COLOR;
    float2 texCoord                 : TEXCOORD0;
    float3 osNormal                 : NORMAL;
    float3 osBinormal               : BINORMAL;
    float3 osTangent                : TANGENT;
*/
};

struct VS_WriteDeferred_Output
{
    float4 depth                    : TEXCOORD0;
    float2 texCoord                 : TEXCOORD1;
    float3 vsNormal                 : TEXCOORD2;
    float3 vsTangent                : TEXCOORD3;
    float3 vsBinormal               : TEXCOORD4;
    float3 wsPosition               : TEXCOORD5;
    float3 wsNormal                 : COLOR1;   // Pack into a color since we have limited texture interpolators
    float3 osPosition               : TEXCOORD7;
    float4 color                    : COLOR0;
    float4 ssPosition               : SV_POSITION;    
};

struct PS_WriteDeferred_Input
{
    float4 depth                    : TEXCOORD0;
    float2 texCoord                 : TEXCOORD1;
    float3 vsNormal                 : TEXCOORD2;
    float3 vsTangent                : TEXCOORD3;
    float3 vsBinormal               : TEXCOORD4;
    float3 wsPosition               : TEXCOORD5;
    float3 wsNormal                 : COLOR1;   // Pack into a color since we have limited texture interpolators
    float3 osPosition               : TEXCOORD7;
    float4 color                    : COLOR0;
};


struct PS_WriteDeferred_Output
{
    float4 albedo                   : SV_Target0;
    float4 normal                   : SV_Target1;
    float4 depth                    : SV_Target2;
};




float3 PackNormal(float3 normal)
{
    return normal * 0.5 + 0.5;
}

float3 UnpackNormal(float3 normal)
{
    return normal * 2 - 1;
}

float4 ConvertNormal(float3 vsNormal)
{
    return float4( PackNormal(vsNormal), 0);
}


VS_WriteDeferred_Output WriteDeferredOutput(
    float2 texCoord,
    float4 color,
    float4 osPosition,
    float3 osNormal,
    float3 osBinormal,
    float3 osTangent,
    float4x4 _objectToWorldMatrix,
    float4x4 _objectToWorldMatrixInvTrans  // Inverse transpose of the object to world matrix
    )
{
    // word space
    float4 wsPosition = mul(osPosition, _objectToWorldMatrix);
    float3 wsNormal   = normalize( mul(osNormal,   (float3x3)_objectToWorldMatrixInvTrans ) );
    float3 wsBinormal = normalize( mul(osBinormal, (float3x3)_objectToWorldMatrixInvTrans ) );
    float3 wsTangent  = normalize( mul(osTangent,  (float3x3)_objectToWorldMatrixInvTrans ) );
    // view space
    float3 vsNormal   = mul(wsNormal, (float3x3)worldToCameraMatrix );
    float3 vsTangent  = mul(wsTangent, (float3x3)worldToCameraMatrix );
    float3 vsBinormal = mul(wsBinormal, (float3x3)worldToCameraMatrix );

    VS_WriteDeferred_Output output;
    
    float vsDepth = wsPosition.x * worldToCameraMatrix[0][2] +
                    wsPosition.y * worldToCameraMatrix[1][2] +
                    wsPosition.z * worldToCameraMatrix[2][2] +  
                    worldToCameraMatrix[3][2];

    output.ssPosition   = mul(wsPosition, worldToScreenMatrix);
    output.depth        = float4( vsDepth, 0, 0, 0 );
    output.texCoord     = texCoord;
    output.vsNormal     = vsNormal;
    output.vsBinormal   = vsBinormal;
    output.vsTangent    = vsTangent;
    output.color        = color;
    output.wsPosition = wsPosition.xyz / wsPosition.w;
    output.osPosition = osPosition.xyz;
    output.wsNormal = PackNormal(wsNormal);

    return output;
}


VS_WriteDeferred_Output WriteDeferredVS(VS_WriteDeferred_Input input)
{
    float4x4 objectToWorldMatrixInvTrans = objectToWorldMatrix;

    float4 osPosition  = float4(input.osPosition, 1);
    float3 osNormal    = float3(1,1,1); // input.osNormal;
    float3 osBinormal  = float3(1,1,1); // input.osBinormal;
    float3 osTangent   = float3(1,1,1); // input.osTangent;

    return WriteDeferredOutput(input.texCoord, input.color, osPosition, osNormal, osBinormal, osTangent, objectToWorldMatrix, objectToWorldMatrixInvTrans);
}


PS_WriteDeferred_Output DeferredShadingPS(PS_WriteDeferred_Input input)
{
    PS_WriteDeferred_Output output;
    
    // Transform the normal into view space.
    float3 vsNormal = normalize(input.vsTangent +
                                input.vsBinormal +
                                input.vsNormal);  

    output.normal          = ConvertNormal(vsNormal);
    output.depth           = float4(input.depth.r, input.depth.g, 0, 0);
    output.albedo 		= float4(1,0,0,1);
    return output;
}









