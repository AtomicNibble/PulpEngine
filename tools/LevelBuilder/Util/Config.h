#pragma once

X_NAMESPACE_BEGIN(level)

static const int PLANENUM_LEAF = -1;
static const int PLANENUM_AREA_DIFF = -2;

static const size_t BSP_TREE_BLOCK_SIZE = 1024;
static const size_t MAX_PRIMATIVES = 1 << 17;

// Epsilon's
static const float ON_EPSILON = 0.1f;
static const float CLIP_EPSILON = 0.1f;
static const float COPLANAR_EPSILON = 0.1f;

static const float PLANE_NORMAL_EPSILON = 0.00001f;
static const float PLANE_DIST_EPSILON = 0.01f;

static const float BASE_WINDING_EPSILON = 0.001f;
static const float SPLIT_WINDING_EPSILON = 0.001f;

static const float DEFAULT_CURVE_MAX_ERROR = 4.0f;
static const float DEFAULT_CURVE_MAX_LENGTH = -1.0f;

X_NAMESPACE_END