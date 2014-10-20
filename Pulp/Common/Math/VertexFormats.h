#pragma once


#ifndef X_VERTEX_FORMATS_H_
#define X_VERTEX_FORMATS_H_

#include <Util\EnumMacros.h>
#include <Math\XVecCompressed.h>


struct Vertex_P3F_C4B
{
	Vec3f pos;
	Color8u color;
};


struct Vertex_P3F_C4B_T2F // mainly for GUI.
{
	Vec3f pos;
	Color8u color;
	Vec2<float> st;
};

struct Vertex_P3F_C4B_T2S
{
	Vec3f pos;
	Color8u color;
	Vec2<XHalf> st;
};

struct Vertex_P3F_T3F
{
	Vec3f pos;
	Vec3f st;
};

struct Vertex_P3F_N10_C4B_T2S
{
	Vec3f pos;
	uint32_t normal; // 10|10|10|2 normals
	Color8u color;
	Vec2<XHalf> st;
};


struct Vertex_P3F_T4F_N3F_C4B
{
	Vec3f		pos;			// Vertex position.
	Vec2f		texcoord[2];	// Vertex texture coordinates. 0 = surface, 1 = lightmap.
	Vec3f		normal;			// Vertex normal.
	Color8u		color;			// RGBA col baby. tickle my alpha channel.
};




struct Vertex_Tangents
{
	compressedVec3 tangent;
};

struct Vertex_Tangents_BiNorm
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

typedef Vertex_P3F_C4B_T2F XAuxVertex;

X_DECLARE_ENUM(VertexStream)(VERT, SKIN_INFO, TANGET, TANGENT_BI);

#endif // !X_VERTEX_FORMATS_H_