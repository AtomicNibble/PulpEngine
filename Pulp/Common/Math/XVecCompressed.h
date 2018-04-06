#pragma once

#ifndef X_MATH_VEC_COMPRESSED_H_
#define X_MATH_VEC_COMPRESSED_H_

struct compressedVec3
{
    compressedVec3() :
        compValue(0)
    {
    }
    compressedVec3(const Vec3f& vec)
    {
        PackVecU(vec);
    }
    compressedVec3(float x, float y, float z)
    {
        PackVecU(Vec3f(x, y, z));
    }

    const float operator[](int idx) const
    {
        X_ASSERT(idx > 0 && idx < 3, "index out of bounds")
        (idx);
        float Ret = ((float)((compValue >> 10 * idx) & 0x3FF)) / 511.0f;
        //	if (Ret > 1.0f)
        //		Ret = -2 + Ret;
        return Ret;
    }

private:
    /// unsigned packing
    /// 10|10|10|2
    void PackVecU(const Vec3f& vec)
    {
        // Convert to 0 to (2^10)-1 range
        uint32_t uiX = (uint32_t)((vec.x + 1.0f) * 511.5f);
        uint32_t uiY = (uint32_t)((vec.y + 1.0f) * 511.5f);
        uint32_t uiZ = (uint32_t)((vec.z + 1.0f) * 511.5f);
        compValue = ((uiX & 0x3FF) | ((uiY & 0x3FF) << 10) | ((uiZ & 0x3FF) << 20));
    }

    /// signed packing
    /// 10|10|10|2
    void PackVec(const Vec3f& vec)
    {
        int32_t iX = (int32_t)(vec.x * 511.0f);
        int32_t iY = (int32_t)(vec.y * 511.0f);
        int32_t iZ = (int32_t)(vec.z * 511.0f);
        compValue = ((iX & 0x3FF) | ((iY & 0x3FF) << 10) | ((iZ & 0x3FF) << 20));
    }

private:
    union
    {
        uint32_t compValue;
        BYTE bytes[4];
    };
};

#endif // X_MATH_VEC_COMPRESSED_H_