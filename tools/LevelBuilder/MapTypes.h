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
	const core::string& getLayer(void) const { return layer_; }

	const bool hasLayer(void) const { return layer_.isNotEmpty(); }

protected:
	core::string layer_;
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

	size_t				GetNumSides(void) const { return sides.size(); }
	void				AddSide(XMapBrushSide *side) { sides.push_back(side); }
	XMapBrushSide*		GetSide(size_t i) const { return sides[i]; }

public:
	static XMapBrush*	Parse(XLexer& src, core::MemoryArenaBase* arena, const Vec3f& origin);

protected:
	core::Array<XMapBrushSide*> sides;

};

class XMapPatch : public XMapPrimitive
{
public:
	XMapPatch(void);
	XMapPatch(int w, int h);
	~XMapPatch(void);


	X_INLINE void SetHorzSubdivisions(size_t num) { horzSubdivisions_ = num; }
	X_INLINE void SetVertSubdivisions(size_t num) { vertSubdivisions_ = num; }
	X_INLINE size_t GetHorzSubdivisions(void) const { return horzSubdivisions_; }
	X_INLINE size_t GetVertSubdivisions(void) const { return vertSubdivisions_; }

	X_INLINE size_t GetNumIndexes(void) const { return indexes_.size(); }
	X_INLINE const int* GetIndexes(void) const { return indexes_.ptr(); }

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

	void CreateNormalsAndIndexes(void);

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
	bool _pad[2];
};


class IgnoreList
{
	typedef core::Array<core::string> IgnoreArray;

public:
	IgnoreList(IgnoreArray& ignoreList) : ignoreList_(ignoreList) {}

	bool isIgnored(const core::string& layerName) const 
	{
		IgnoreArray::ConstIterator it = ignoreList_.begin();
		for (; it != ignoreList_.end(); ++it)
		{
			if (*it == layerName) {
				return true;
			}
		}
		return false;
	}

private:
	IgnoreArray ignoreList_;
};


class XMapEntity
{
public:
	typedef KeyPair PairMap;
	typedef KeyPair::PairIt PairIt;

public:
	XMapEntity(void) : primitives(g_arena) {}
	~XMapEntity(void) {}

	size_t					GetNumPrimitives(void) const { return primitives.size(); }
	XMapPrimitive*		GetPrimitive(int i) const { return primitives[i]; }
	void				AddPrimitive(XMapPrimitive *p) { primitives.push_back(p); }

public:
	static XMapEntity*	Parse(XLexer &src, core::MemoryArenaBase* arena,
		const IgnoreList& ignoredLayers, bool isWorldSpawn = false);


	PairMap epairs;

protected:
	core::Array<XMapPrimitive*>	primitives;
};

#ifdef IGNORE
#undef IGNORE
#endif // !IGNORE

X_DECLARE_FLAGS(LayerFlag)(ACTIVE, EXPANDED, IGNORE);

struct Layer
{
	typedef Flags<LayerFlag> LayerFlags;

	core::string name;
	LayerFlags flags;
};

X_NAMESPACE_END

#include "MapTypes.inl"

#endif // X_MAP_TYPES_H_