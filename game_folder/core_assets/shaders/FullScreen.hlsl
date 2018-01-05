

//
// Draw 3 null tri strip verts, to make fullscreen quad.
//

#if X_TEXTURED
Texture2D           colorMap : register(t0);
SamplerState        colorSampler;
#endif // !X_TEXTURED


struct VS_OUTPUT
{
    float4 ssPosition               : SV_POSITION;

#if X_TEXTURED
    float2 texCoord                 : TEXCOORD0;
#endif // !X_TEXTURED
};

struct PS_OUTPUT
{
    float4 color                    : SV_TARGET0;
};


VS_OUTPUT vs_main( uint vertexID : SV_VertexID )
{
    VS_OUTPUT result;

    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);

#if X_TEXTURED
    result.texCoord = uv;
#endif // !X_TEXTURED

    result.ssPosition = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return result;
}

PS_OUTPUT ps_main( VS_OUTPUT IN )
{
    PS_OUTPUT output;
    output.color = float4(1,1,1,1);

#if X_TEXTURED
    float4 texCol = colorMap.Sample(colorSampler, IN.texCoord);
    output.color = output.color * texCol;
#endif // !X_TEXTURED

    return output;
}


float LinearizeDepth(float2 uv)
{
    float zNear = 0.5;
    float zFar  = 2048.0;
    float depth = colorMap.Sample(colorSampler, uv).x;

    return depth * 20;
    // depth = 1.f - depth;
    //return 1.f - (depth * 25);
//    return (2.0 * zNear) / (zFar + zNear - depth * (zFar - zNear));
}

PS_OUTPUT ps_depth( VS_OUTPUT IN )
{
    PS_OUTPUT output;
    output.color = float4(1,1,1,1);

#if X_TEXTURED
    float d = LinearizeDepth(IN.texCoord);
    output.color = float4(d,0,0,1);

#endif // !X_TEXTURED

    return output;
}