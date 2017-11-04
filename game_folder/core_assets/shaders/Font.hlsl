
#include "Vertexbase.inc"

// All fonts are drawn with P3F_T2F_C4B
struct VS_INPUT
{
    float3 osPosition           	: POSITION;
    float2 texCoord             	: TEXCOORD0;
    float4 color                	: COLOR0;
};

struct VS_OUTPUT
{
    float4 ssPosition           	: SV_POSITION;
    float2 texCoord             	: TEXCOORD;
    float4 color                	: COLOR;
};

struct PS_INPUT
{
    float4 ssPosition           	: SV_POSITION;
    float2 texCoord             	: TEXCOORD0;
    float4 color                	: COLOR0;
};

struct PS_OUTPUT
{
    float4 color                : SV_TARGET0;
};


Texture2D  	       fontCache : register(t0);
SamplerState        fontCacheSampler;


VS_OUTPUT BasicVS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.ssPosition        	= mul( float4(input.osPosition, 1.0), worldToScreenMatrix );
    output.texCoord          	= input.texCoord;
    output.color             	= input.color;
    return output;
}


float contour( in float d, in float w ){
    return smoothstep(0.5 - w, 0.5 + w, d);
}
float samp( in float2 uv, float w ){
    return contour(fontCache.Sample(fontCacheSampler, uv).a, w );
}

PS_OUTPUT FontPS(PS_INPUT input)
{
    PS_OUTPUT output;
    output.color = input.color;

#if 0 // enable supersample.
    float2 uv = input.texCoord;
    float dist = fontCache.Sample(fontCacheSampler, uv).a;
    float width = fwidth(dist);
    float alpha = contour( dist, width );

    float dscale = 0.354; // half of 1/sqrt2; you can play with this
    float2 duv = dscale * (ddx(uv) + ddy(uv));
    float4 box = float4(uv-duv, uv+duv);

    float asum = samp( box.xy, width )
               + samp( box.zw, width )
               + samp( box.xw, width )
               + samp( box.zy, width );

    // weighted average, with 4 extra points having 0.5 weight each,
    // so 1 + 0.5*4 = 3 is the divisor
    alpha = (alpha + 0.5 * asum) / 3.;

    output.color.a *= alpha;
#else
    float dist = fontCache.Sample(fontCacheSampler, input.texCoord).a;
    float edgeDistance = 0.5;
    float edgeWidth = 0.7 * length(float2(ddx(dist), ddy(dist)));
    float opacity = smoothstep(edgeDistance - edgeWidth, edgeDistance + edgeWidth, dist);

    output.color.a *= opacity;
#endif
    return output;
}
