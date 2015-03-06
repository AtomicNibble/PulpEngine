#pragma once


#ifndef X_VERTEX_FORMATS_H_
#define X_VERTEX_FORMATS_H_

#include <Util\EnumMacros.h>
#include <Math\XVecCompressed.h>

//tghey eed to be in the end of the vete format layout tststem that need to be done for the end
//one i've got this vertex format issue sorted i can then begin to endthe rnder system get the 3d engine taking njobs etcthat then can be used
// i need to work towods a job based system as well
// maybe if i get sick just owkr on that for a bit which takes a number of job that can be 
// used for a number of things.
// like rendering somthing and calculating many things at once i'm not really surewhat i would spearte out
// nto jobs tho i knida need more knowlege on the frendering system in order to work out what need to be don for the job system
// maybe i can take a look at one of the id engine source codes to work out what can be made multiple threaded.
// but this input layout thing is driving me around in bends.





struct Vertex_P3F_T3F
{
	Vec3f pos;
	Vec3f st;
};

/*
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



// 16 bit texcoords
struct Vertex_P3F_T2S
{
	Vec3f		pos;	// 12
	Vec2<XHalf> st;		// 4
}; // 16

struct Vertex_P3F_T2S_C4B : public Vertex_P3F_T2S
{
	Color8u		color;
}; // 20



struct Vertex_P3F_T2S_C4B_N3F : public Vertex_P3F_T2S
{
	Color8u		color;
	Vec3f		normal;
}; // 22

struct Vertex_P3F_T2S_C4B_N3F_TB3F : 
public Vertex_P3F_T2S, public Vertex_Tangents_BiNorm
{
	Color8u		color;
	Vec3f		normal;
}; // 



struct Vertex_P3F_T2S_C4B_N10 : public Vertex_P3F_T2S
{
	Color8u		color;
	uint32_t  normal; // 10|10|10|2 normals
}; // 22

struct Vertex_P3F_T2S_C4B_N10_TB20 : 
public Vertex_P3F_T2S, public Vertex_Tangents_BiNorm_Compressed
{
	Color8u		color;
	uint32_t  normal; // 10|10|10|2 normals
}; // 




// 32bit tex cords
struct Vertex_P3F_T2F_C4B
{
	Vec3f		pos;	// 12
	Vec2<float> st;		// 4
	Color8u		color;
}; // 16





typedef Vertex_P3F_T2F_C4B XAuxVertex;

X_DECLARE_ENUM8(VertexStream)(VERT, COLOR, NORMALS, TANGENT_BI);

#endif // !X_VERTEX_FORMATS_H_