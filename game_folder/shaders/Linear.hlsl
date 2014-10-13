
/** Converts from sRGB to linear space */
float3 ConvertToLinear(float3 c)
{
    const float a = 0.055;
    return pow((c + a) / (1.0 + a), 2.4);
}
float4 ConvertToLinear(float4 c)
{
    const float a = 0.055;
    return pow((c + a) / (1.0 + a), 2.4);
}

/** Converts from linear space to sRGB */
float3 ConvertToSRGB(float3 c)
{
    const float a = 0.055;
    return (1 + a) * pow(abs(c), 1.0 / 2.4);
}

