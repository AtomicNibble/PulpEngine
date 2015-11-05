#pragma once

#ifndef X_MAP_TYPES_H_
#define X_MAP_TYPES_H_

using namespace core;

X_NAMESPACE_DECLARE(core, 
struct XFile;
)

X_DECLARE_ENUM(PrimType)(BRUSH, PATCH, INVALID);

X_NAMESPACE_BEGIN(mapfile)


class XMapBrush;



// base class for Brush / Patch
class XMapPrimitive
{
public:
	XMapPrimitive(PrimType::Enum type) : type_(type) { }
	virtual	~XMapPrimitive(void) {}

	PrimType::Enum getType(void) const { return type_; }

protected:
	PrimType::Enum	type_;
};

class XMapBrushSide 
{
	friend class XMapBrush;

public:
	XMapBrushSide(void) {}
	~XMapBrushSide(void) { }
	const char*			GetMaterialName(void) const { return material.name.c_str(); }
	const Planef&		GetPlane(void) const { return plane; }

	struct MaterialInfo
	{
		core::StackString<level::MAP_MAX_MATERIAL_LEN> name;
		Vec2f				  matRepeate;
		Vec2f				  shift;
		float				  rotate;
	};

	MaterialInfo	material;
	MaterialInfo	lightMap;

protected:

	Planef			plane;


protected:
	static bool ParseMatInfo(XLexer& src, MaterialInfo& mat);
};


class XMapBrush : public XMapPrimitive
{
public:
	XMapBrush(void) : XMapPrimitive(PrimType::BRUSH), sides(g_arena) { sides.reserve(6); }
	~XMapBrush(void) X_OVERRIDE {}

	int					GetNumSides(void) const { return safe_static_cast<int,size_t>(sides.size()); }
	void				AddSide(XMapBrushSide *side) { sides.push_back(side); }
	XMapBrushSide*		GetSide(int i) const { return sides[i]; }
	uint32_t			GetGeometryCRC(void) const;

public:
	static XMapBrush*	Parse(XLexer& src, core::MemoryArenaBase* arena, const Vec3f& origin);

protected:
	core::Array<XMapBrushSide*> sides;

};

class XMapPatch : public XMapPrimitive
{
public:
	XMapPatch(void) : XMapPrimitive(PrimType::PATCH),
		verts_(g_arena),
		indexes_(g_arena),
		edges_(g_arena),
		edgeIndexes_(g_arena),
		width_(0), height_(0), 
		maxWidth_(0), maxHeight_(0) 
	{ 
		isMesh_ = false;
		expanded_ = false;
	}
	XMapPatch(int w, int h) : XMapPrimitive(PrimType::PATCH),
		verts_(g_arena),
		indexes_(g_arena),
		edges_(g_arena),
		edgeIndexes_(g_arena),
		width_(w), height_(h),
		maxWidth_(w), maxHeight_(h) 
	{
		isMesh_ = false;
		expanded_ = false;
	}
	~XMapPatch(void) X_OVERRIDE {}


	void SetHorzSubdivisions(size_t num) { horzSubdivisions_ = num; }
	void SetVertSubdivisions(size_t num) { vertSubdivisions_ = num; }
	size_t GetHorzSubdivisions(void) const { return horzSubdivisions_; }
	size_t GetVertSubdivisions(void) const { return vertSubdivisions_; }

	size_t GetNumIndexes(void) const { return indexes_.size(); }
	const int* GetIndexes(void) const { return indexes_.ptr(); }

	X_INLINE const xVert& operator[](const int idx) const {
		return verts_[idx];
	}
	X_INLINE xVert& operator[](const int idx) {
		return verts_[idx];
	}

	X_INLINE void SetMesh(bool b) {
		isMesh_ = b;
	}
	X_INLINE const bool isMesh(void) const {
		return isMesh_;
	}

	X_INLINE const char* GetMatName(void) const {
		return matName_.c_str();
	}

	// Subdived util.
	void Subdivide(float maxHorizontalError, float maxVerticalError, 
		float maxLength, bool genNormals = false);
	void SubdivideExplicit(size_t horzSubdivisions, size_t vertSubdivisions,
		bool genNormals, bool removeLinear = false);

private:
	void PutOnCurve(void);
	void RemoveLinearColumnsRows(void);
	void ResizeExpanded(size_t height, size_t width);
	void Expand(void);
	void Collapse(void);
	void GenerateNormals(void);
	void GenerateIndexes(void);
	void LerpVert(const xVert& a, const xVert& b, xVert& out) const;
	void GenerateEdgeIndexes(void);

	void ProjectPointOntoVector(const Vec3f& point, const Vec3f& vStart, 
		const Vec3f& vEnd, Vec3f& vProj);
	void SampleSinglePatch(const xVert ctrl[3][3], size_t baseCol, size_t baseRow,
		size_t width, size_t horzSub, size_t vertSub, xVert* outVerts) const;
	void SampleSinglePatchPoint(const xVert ctrl[3][3], float u,
		float v, xVert* out) const;

public:
	static XMapPatch* Parse(XLexer &src, core::MemoryArenaBase* arena, const Vec3f &origin);

protected:
	struct surfaceEdge_t
	{
		int32_t	verts[2];	// edge vertices always with ( verts[0] < verts[1] )
		int32_t	tris[2];	// edge triangles
	};

	core::Array<xVert>			verts_;
	core::Array<int>			indexes_;	// 3 references to vertices for each triangle
	core::Array<surfaceEdge_t>	edges_;		// edges
	core::Array<int>			edgeIndexes_;

	core::StackString<level::MAP_MAX_MATERIAL_LEN> matName_;
	core::StackString<level::MAP_MAX_MATERIAL_LEN> lightMap_;

	size_t width_;
	size_t height_;
	size_t maxWidth_;
	size_t maxHeight_;
	size_t horzSubdivisions_;
	size_t vertSubdivisions_;

	bool isMesh_;
	bool expanded_;		
};



class XMapEntity
{
public:
	XMapEntity(void) : primitives(g_arena) {}
	~XMapEntity(void) {}

	int					GetNumPrimitives(void) const { return (int)primitives.size(); }
	XMapPrimitive*		GetPrimitive(int i) const { return primitives[i]; }
	void				AddPrimitive(XMapPrimitive *p) { primitives.push_back(p); }
	uint32_t			GetGeometryCRC(void) const;
	void				RemovePrimitiveData();

public:
	static XMapEntity*	Parse(XLexer &src, core::MemoryArenaBase* arena, bool isWorldSpawn = false);

	typedef KeyPair PairMap;
	typedef KeyPair::PairIt PairIt;

	PairMap epairs;

protected:
	core::Array<XMapPrimitive*>	primitives;
};

X_NAMESPACE_END


#endif // X_MAP_TYPES_H_