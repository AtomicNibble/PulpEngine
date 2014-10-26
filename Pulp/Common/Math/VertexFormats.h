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

struct Vertex_P3F_N10_C4B_T2S
{
	Vec3f pos;
	uint32_t normal; // 10|10|10|2 normals
	Color8u color;
	Vec2<XHalf> st;
};

struct Vertex_P3F_N3F_C4B_T4F
{
	Vec3f		pos;			// Vertex position.
	Vec3f		normal;			// Vertex normal.
	Color8u		color;			// RGBA col baby. tickle my alpha channel.
	Vec2f		texcoord[2];	// Vertex texture coordinates. 0 = surface, 1 = lightmap.
};

// 16 bit texcoords
struct Vertex_P3F_T2S
{
	Vec3f		pos;	// 12
	Vec2<XHalf> st;		// 4
}; // 16

struct Vertex_P3F_T2S_C4B
{
	Vec3f		pos;	// 12
	Vec2<XHalf> st;		// 4
	Color8u		color;
}; // 20


struct Vertex_P3F_T2S_C4B_N3F
{
	Vec3f		pos;	// 12
	Vec2<XHalf> st;		// 4
	Color8u		color;
	Vec3f		normal;
}; // 22

struct Vertex_P3F_T2S_C4B_N3F_TB3F
{
	Vec3f		pos;	// 12
	Vec2<XHalf> st;		// 4
	Color8u		color;
	Vec3f		normal;
	Vec3f		tangent;
	Vec3f		binormal;
}; // 


struct Vertex_P3F_T2F_C4B
{
	Vec3f		pos;	// 12
	Vec2<float> st;		// 4
	Color8u		color;
}; // 16


/*

struct Vertex_P3F_C4B
{
	Vec3f pos;
	Color8u color;
};


struct Vertex_P3F_T2S_N3F 
{
	Vec3f		pos;	// 12
	Vec2<XHalf> st;		// 4
	Vec3f		normal;
}; // 16 + 12

struct Vertex_P3F_T2S_N3F_TB3F 
{
	Vec3f		pos;	// 12
	Vec2<XHalf> st;		// 4
	Vec3f		normal;
	Vec3f		tangent;
	Vec3f		binormal;
}; // 16 + 12 


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

*/

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

typedef Vertex_P3F_T2F_C4B XAuxVertex;

X_DECLARE_ENUM8(VertexStream)(VERT, COLOR, NORMALS, TANGENT_BI);

#endif // !X_VERTEX_FORMATS_H_