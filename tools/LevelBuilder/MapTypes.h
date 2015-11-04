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
		verts(g_arena),
		indexes(g_arena),
		edges(g_arena),
		edgeIndexes(g_arena),
		width(0), height(0), 
		maxWidth(0), maxHeight(0) 
	{ 
		isMesh_ = false;
		expanded = false;
	}
	XMapPatch(int w, int h) : XMapPrimitive(PrimType::PATCH),
		verts(g_arena),
		indexes(g_arena),
		edges(g_arena),
		edgeIndexes(g_arena),
		width(w), height(h),
		maxWidth(w), maxHeight(h) 
	{
		isMesh_ = false;
		expanded = false;
	}
	~XMapPatch(void) X_OVERRIDE {}


	void	SetHorzSubdivisions(int n) { horzSubdivisions = n; }
	void	SetVertSubdivisions(int n) { vertSubdivisions = n; }
	int		GetHorzSubdivisions(void) const { return horzSubdivisions; }
	int		GetVertSubdivisions(void) const { return vertSubdivisions; }

	int			GetNumIndexes(void) const { return (int)indexes.size(); }
	const int*	GetIndexes(void) const { return indexes.ptr(); }

	X_INLINE const xVert& operator[](const int idx) const {
		return verts[idx];
	}
	X_INLINE xVert& operator[](const int idx) {
		return verts[idx];
	}

	X_INLINE void SetMesh(bool b) {
		isMesh_ = b;
	}
	X_INLINE const bool isMesh(void) const {
		return isMesh_;
	}

	X_INLINE const char* GetMatName(void) const {
		return matName.c_str();
	}

	// Subdived util.
	void Subdivide(float maxHorizontalError, float maxVerticalError, 
		float maxLength, bool genNormals = false);
	void SubdivideExplicit(int horzSubdivisions, int vertSubdivisions,
		bool genNormals, bool removeLinear = false);


private:
	void	PutOnCurve(void);
	void	RemoveLinearColumnsRows(void);
	void	ResizeExpanded(int height, int width);
	void	Expand(void);
	void	Collapse(void);
	void	GenerateNormals(void);
	void	GenerateIndexes(void);
	void	LerpVert(const xVert& a, const xVert& b, xVert& out) const;
	void	GenerateEdgeIndexes(void);

	void ProjectPointOntoVector(const Vec3f& point, const Vec3f& vStart, 
		const Vec3f& vEnd, Vec3f& vProj);
	void SampleSinglePatch(const xVert ctrl[3][3], int baseCol, int baseRow,
		int width, int horzSub, int vertSub, xVert* outVerts) const;
	void SampleSinglePatchPoint(const xVert ctrl[3][3], float u,
		float v, xVert* out) const;

public:
	static XMapPatch* Parse(XLexer &src, core::MemoryArenaBase* arena, const Vec3f &origin);

protected:
	typedef struct surfaceEdge_s {
		int						verts[2];	// edge vertices always with ( verts[0] < verts[1] )
		int						tris[2];	// edge triangles
	} surfaceEdge_t;


	core::Array<xVert>				verts;
	core::Array<int>				indexes;	// 3 references to vertices for each triangle
	core::Array<surfaceEdge_t>		edges;		// edges
	core::Array<int>				edgeIndexes;

	core::StackString<level::MAP_MAX_MATERIAL_LEN> matName;
	core::StackString<level::MAP_MAX_MATERIAL_LEN> lightMap;

	int		width, height;
	int		maxWidth, maxHeight;
	int		horzSubdivisions;
	int		vertSubdivisions;

	bool	isMesh_;
	bool	expanded;		
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