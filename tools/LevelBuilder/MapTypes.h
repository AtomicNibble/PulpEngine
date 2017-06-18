#pragma once

#ifndef X_MAP_TYPES_H_
#define X_MAP_TYPES_H_

#ifdef IGNORE
#undef IGNORE
#endif // !IGNORE




X_NAMESPACE_DECLARE(core, 
	struct XFile;
)


X_NAMESPACE_BEGIN(mapfile)

X_DECLARE_ENUM(PrimType)(BRUSH, PATCH, INVALID);
X_DECLARE_FLAGS(LayerFlag)(ACTIVE, EXPANDED, IGNORE);

class XMapBrush;

// base class for Brush / Patch
class XMapPrimitive
{
public:
	X_INLINE XMapPrimitive(PrimType::Enum type);
	X_INLINE virtual ~XMapPrimitive(void);

	X_INLINE PrimType::Enum getType(void) const;
	X_INLINE const core::string& getLayer(void) const;

	X_INLINE const bool hasLayer(void) const;

protected:
	core::string layer_;
	PrimType::Enum type_;
};

class XMapBrushSide 
{
	friend class XMapBrush;

public:
	struct MaterialInfo
	{
		X_INLINE MaterialInfo();

		bool ParseMatInfo(core::XLexer& src);

		core::StackString<level::MAP_MAX_MATERIAL_LEN> name;
		Vec2f				  matRepeate;
		Vec2f				  shift;
		float				  rotate;
	};

public:
	X_INLINE XMapBrushSide(void);
	X_INLINE ~XMapBrushSide(void);

	X_INLINE const char* GetMaterialName(void) const;
	X_INLINE const Planef& GetPlane(void) const;
	X_INLINE const MaterialInfo& GetMaterial(void) const;
	X_INLINE const MaterialInfo& GetLightMap(void) const;

	bool ParseMatInfo(core::XLexer& src);

protected:
	MaterialInfo	material_;
	MaterialInfo	lightMap_;
	Planef			plane_;
};


class XMapBrush : public XMapPrimitive
{
public:
	X_INLINE XMapBrush(void);
	X_INLINE ~XMapBrush(void) X_OVERRIDE;

	X_INLINE size_t	GetNumSides(void) const;
	X_INLINE void AddSide(XMapBrushSide* pSide);
	X_INLINE XMapBrushSide*	GetSide(size_t i) const;

public:
	static XMapBrush* Parse(core::XLexer& src, core::MemoryArenaBase* arena, const Vec3f& origin);

protected:
	core::Array<XMapBrushSide*> sides;
};

class XMapPatch : public XMapPrimitive
{
public:
	XMapPatch(void);
	XMapPatch(int w, int h);
	~XMapPatch(void) X_OVERRIDE;

	X_INLINE void SetHorzSubdivisions(size_t num);
	X_INLINE void SetVertSubdivisions(size_t num);
	X_INLINE size_t GetHorzSubdivisions(void) const;
	X_INLINE size_t GetVertSubdivisions(void) const;

	X_INLINE size_t GetNumIndexes(void) const;
	X_INLINE const int* GetIndexes(void) const;

	X_INLINE const xVert& operator[](const int idx) const;
	X_INLINE xVert& operator[](const int idx);

	X_INLINE void SetMesh(bool b);
	X_INLINE const bool isMesh(void) const;
	X_INLINE const char* GetMatName(void) const;

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
	static XMapPatch* Parse(core::XLexer& src, core::MemoryArenaBase* arena, const Vec3f &origin);

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
	X_INLINE explicit IgnoreList(const IgnoreArray& ignoreList);
	X_INLINE explicit IgnoreList(IgnoreArray&& ignoreList);

	X_INLINE bool isIgnored(const core::string& layerName) const;

private:
	IgnoreArray ignoreList_;
};


class XMapEntity
{
public:
	typedef core::Array<XMapPrimitive*> PrimativeArry;
	typedef KeyPair PairMap;
	typedef KeyPair::PairIt PairIt;

public:
	X_INLINE XMapEntity(void);
	X_INLINE ~XMapEntity(void);

	X_INLINE size_t GetNumPrimitives(void) const;
	X_INLINE XMapPrimitive* GetPrimitive(size_t i) const;
	X_INLINE void AddPrimitive(XMapPrimitive* p);

public:
	static XMapEntity* Parse(core::XLexer& src, core::MemoryArenaBase* arena,
		const IgnoreList& ignoredLayers, bool isWorldSpawn = false);


	PairMap epairs;

protected:
	core::MemoryArenaBase* primArena_;
	PrimativeArry	primitives;
};


struct Layer
{
	typedef Flags<LayerFlag> LayerFlags;

	core::string name;
	LayerFlags flags;
};

X_NAMESPACE_END

#include "MapTypes.inl"

#endif // X_MAP_TYPES_H_