#pragma once

#ifndef X_BSP_VERTEX_H_
#define X_BSP_VERTEX_H_

// vertex format used by the compiler.

struct LvlVert
{
	Vec3f pos;
	Vec2f uv;
	Vec3f normal;
	Vec4<uint8> color;
};


#endif // !X_BSP_VERTEX_H_

