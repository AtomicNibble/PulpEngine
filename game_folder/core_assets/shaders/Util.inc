//--------------------------------------------------------------
//  Version: 1.0
//  MadeBy: WinCat
//  Site: tom-crowley.co.uk
//--------------------------------------------------------------



float4x4 CreateMatrixFromRows(float4 r0, float4 r1, float4 r2, float4 r3) {
    return float4x4(r0, r1, r2, r3);
}

float4x4 CreateMatrixFromCols(float4 c0, float4 c1, float4 c2, float4 c3) {
    return float4x4(c0.x, c1.x, c2.x, c3.x,
                    c0.y, c1.y, c2.y, c3.y,
                    c0.z, c1.z, c2.z, c3.z,
                    c0.w, c1.w, c2.w, c3.w);
}


float4 VisualizeNormal(float3 normal)
{
    return float4((normal + 1) * 0.5, 1);
}

float4 VisualizeNormalWorldSpace(float3 vsNormal)
{
    float3 wsNormal = mul((float3x3)cameraToWorldMatrix, vsNormal);
    return float4((wsNormal + 1) * 0.5, 1);
}
