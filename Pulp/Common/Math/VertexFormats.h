#pragma once

#ifndef X_VERTEX_FORMATS_H_
#define X_VERTEX_FORMATS_H_

#include <Util\EnumMacros.h>
#include <Math\XVecCompressed.h>

struct Vertex_P3F_T3F
{
    Vec3f pos;
    Vec3f st;
};

struct Vertex_Tangents_BiNorm
{
    Vec3f tangent;
    Vec3f binormal;
};

struct Vertex_Tangents_BiNorm_Compressed
{
    compressedVec3 tangent;
    compressedVec3 binormal;
};

//  4 bones per vert.
struct Vertex_W4B_I4B
{
    Vec4<uint8_t> weights;
    Vec4<uint8_t> bones;
};

// 16 bit texcoords
struct Vertex_P3F_T2S
{
    Vec3f pos;                    // 12
    X_NAMESPACE(core)::XHalf2 st; // 4
};                                // 16

struct Vertex_P3F_T2S_C4B : public Vertex_P3F_T2S
{
    Color8u color;
}; // 20

struct Vertex_P3F_T2S_C4B_N3F : public Vertex_P3F_T2S_C4B
{
    Vec3f normal;
}; // 22

struct Vertex_P3F_T2S_C4B_N3F_TB3F : public Vertex_P3F_T2S_C4B
{
    Vec3f normal;
    Vec3f tangent;
    Vec3f binormal;
}; //

struct Vertex_P3F_T2S_C4B_N10 : public Vertex_P3F_T2S_C4B
{
    compressedVec3 normal; // 10|10|10|2 normals
};                         // 22

struct Vertex_P3F_T2S_C4B_N10_TB10 : public Vertex_P3F_T2S_C4B
{
    compressedVec3 normal; // 10|10|10|2 normals
    compressedVec3 tangent;
    compressedVec3 binormal;
}; //

// 32bit tex cords
struct Vertex_P3F_T2F_C4B
{
    Vec3f pos; // 12
    Vec2f st;  // 4
    Color8u color;
}; // 16

struct Vertex_P3F_T4F
{
    Vec3f pos;         // 12
    Vec2f texcoord[2]; // Vertex texture coordinates. 0 = surface, 1 = lightmap.
};

struct Vertex_P3F_T4F_C4B_N3F : public Vertex_P3F_T4F
{
    Color8u color;
    Vec3f normal;
};

struct InstancedData_MAT44_C4F
{
    Matrix44f mat;
    Color8u color;
};

struct Vertex_SkinData
{
    Vec4<uint8_t> indexes;
    Vec4<uint16_t> weights;
};

typedef Vertex_P3F_T2F_C4B XAuxVertex;

X_DECLARE_ENUM8(VertexStream)
(VERT, COLOR, NORMALS, TANGENT_BI, HWSKIN, INSTANCE);

// hwskin and instanced are runtime only.
static const size_t VERT_RUNTIME_STREAM_COUNT = 1;

// check sizes.
X_ENSURE_SIZE(Vertex_P3F_T3F, 24);
X_ENSURE_SIZE(Vertex_P3F_T2S, 16);
X_ENSURE_SIZE(Vertex_P3F_T2S_C4B, 20);
X_ENSURE_SIZE(Vertex_P3F_T2S_C4B_N3F, 32);
X_ENSURE_SIZE(Vertex_P3F_T2S_C4B_N3F_TB3F, 32 + 24);
X_ENSURE_SIZE(Vertex_P3F_T2S_C4B_N10, 24);
X_ENSURE_SIZE(Vertex_P3F_T2S_C4B_N10_TB10, 32);
X_ENSURE_SIZE(Vertex_P3F_T2F_C4B, 24);
X_ENSURE_SIZE(Vertex_P3F_T4F_C4B_N3F, 44);

X_ENSURE_SIZE(InstancedData_MAT44_C4F, 68);

#endif // !X_VERTEX_FORMATS_H_