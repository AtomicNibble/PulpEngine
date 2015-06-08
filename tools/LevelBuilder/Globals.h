#pragma once 


#ifndef LVL_BUILDER_GLOBALS_H_
#define LVL_BUILDER_GLOBALS_H_

static const int PLANENUM_LEAF = -1;
static const int PLANENUM_AREA_DIFF = -2;


static const float ON_EPSILON = 0.1f;
static const float CLIP_EPSILON = 0.1f;

static const float BASE_WINDING_EPSILON	= 0.001f;
static const float SPLIT_WINDING_EPSILON	= 0.001f;

static const size_t BSP_TREE_BLOCK_SIZE = 1024;


static const float PLANE_NORMAL_EPSILON = 0.00001f;
static const float PLANE_DIST_EPSILON = 0.01f;

static const float DEFAULT_CURVE_MAX_ERROR = 4.0f;
static const float DEFAULT_CURVE_MAX_LENGTH = -1.0f;

template<typename T>
T* Alloca16(size_t num)
{
	void* pData = _alloca(num + 15);
	// align.
	pData = core::pointerUtil::AlignTop(pData, 16);
	return reinterpret_cast<T*>(pData);
}


#endif // !LVL_BUILDER_GLOBALS_H_