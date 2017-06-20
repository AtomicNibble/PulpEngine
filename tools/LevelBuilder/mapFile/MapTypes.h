#pragma once

#ifndef X_MAP_TYPES_H_
#define X_MAP_TYPES_H_

#ifdef IGNORE
#undef IGNORE
#endif // !IGNORE




X_NAMESPACE_DECLARE(core, 
	struct XFile;
)


X_NAMESPACE_BEGIN(lvl)

namespace mapFile
{


X_DECLARE_ENUM(PrimType)(BRUSH, PATCH);
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
public:
	struct MaterialInfo
	{
		X_INLINE MaterialInfo();

		bool ParseMatInfo(core::XLexer& src);

		MaterialName	name;
		Vec2f			matRepeate;
		Vec2f			shift;
		float			rotate;
	};

public:
	X_INLINE XMapBrushSide(void);
	X_INLINE ~XMapBrushSide(void);

	X_INLINE const char* GetMaterialName(void) const;
	X_INLINE const Planef& GetPlane(void) const;
	X_INLINE const MaterialInfo& GetMaterial(void) const;
	X_INLINE const MaterialInfo& GetLightMap(void) const;

	void SetPlane(const Planef& plane);
	bool ParseMatInfo(core::XLexer& src);

private:
	MaterialInfo	material_;
	MaterialInfo	lightMap_;
	Planef			plane_;
};


class XMapBrush : public XMapPrimitive
{
	typedef core::Array<XMapBrushSide*> BrushSidePtrArr;
public:
	X_INLINE XMapBrush(core::MemoryArenaBase* arena, core::MemoryArenaBase* primArena);
	X_INLINE ~XMapBrush(void) X_OVERRIDE;

	X_INLINE size_t	GetNumSides(void) const;
	X_INLINE void AddSide(XMapBrushSide* pSide);
	X_INLINE XMapBrushSide*	GetSide(size_t i) const;

public:
	 bool Parse(core::XLexer& src, const Vec3f& origin);

private:
	core::MemoryArenaBase* primArena_;
	BrushSidePtrArr sides_;
};

class XMapPatch : public XMapPrimitive
{
	struct SurfaceEdge
	{
		int32_t	verts[2];	// edge vertices always with ( verts[0] < verts[1] )
		int32_t	tris[2];	// edge triangles
	};

	typedef core::Array<LvlVert>			VertArr;
	typedef core::Array<int>			IntArr;	
	typedef core::Array<SurfaceEdge>	SurfaceEdgeArr;	

public:
	XMapPatch(core::MemoryArenaBase* arena);
	XMapPatch(core::MemoryArenaBase* arena, int w, int h);
	~XMapPatch(void) X_OVERRIDE;

	X_INLINE void SetHorzSubdivisions(size_t num);
	X_INLINE void SetVertSubdivisions(size_t num);
	X_INLINE size_t GetHorzSubdivisions(void) const;
	X_INLINE size_t GetVertSubdivisions(void) const;

	X_INLINE size_t GetNumIndexes(void) const;
	X_INLINE const int* GetIndexes(void) const;

	X_INLINE const LvlVert& operator[](const int idx) const;
	X_INLINE LvlVert& operator[](const int idx);

	X_INLINE void SetMesh(bool b);
	X_INLINE const bool isMesh(void) const;
	X_INLINE const char* GetMatName(void) const;

	// Subdived util.
	void Subdivide(float maxHorizontalError, float maxVerticalError, 
		float maxLength, bool genNormals = false);
	void SubdivideExplicit(size_t horzSubdivisions, size_t vertSubdivisions,
		bool genNormals, bool removeLinear = false);

	void CreateNormalsAndIndexes(void);
	
	bool Parse(core::XLexer& src, const Vec3f &origin);

private:
	void PutOnCurve(void);
	void RemoveLinearColumnsRows(void);
	void ResizeExpanded(size_t height, size_t width);
	void Expand(void);
	void Collapse(void);
	void GenerateNormals(void);
	void GenerateIndexes(void);
	void LerpVert(const LvlVert& a, const LvlVert& b, LvlVert& out) const;
	void GenerateEdgeIndexes(void);

	void ProjectPointOntoVector(const Vec3f& point, const Vec3f& vStart, 
		const Vec3f& vEnd, Vec3f& vProj);
	void SampleSinglePatch(const LvlVert ctrl[3][3], size_t baseCol, size_t baseRow,
		size_t width, size_t horzSub, size_t vertSub, LvlVert* outVerts) const;
	void SampleSinglePatchPoint(const LvlVert ctrl[3][3], float u,
		float v, LvlVert* out) const;

protected:
	VertArr			verts_;
	IntArr			indexes_;	// 3 references to vertices for each triangle
	SurfaceEdgeArr	edges_;		// edges
	IntArr			edgeIndexes_;

	core::string matName_;
	core::string lightMap_;

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

	X_INLINE void add(const core::string& layerName);
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
	typedef std::array<size_t,PrimType::ENUM_COUNT> PrimTypeNumArr;

public:
	X_INLINE XMapEntity(core::MemoryArenaBase* arena, core::MemoryArenaBase* primArena);
	X_INLINE ~XMapEntity(void);

	X_INLINE size_t GetNumPrimitives(void) const;
	X_INLINE const PrimTypeNumArr& getPrimCounts(void) const;

	X_INLINE XMapPrimitive* GetPrimitive(size_t i) const;
	X_INLINE void AddPrimitive(XMapPrimitive* p);

public:
	bool Parse(core::XLexer& src, const IgnoreList& ignoredLayers, 
		bool isWorldSpawn = false);

	PairMap epairs;

private:
	core::MemoryArenaBase* arena_;
	core::MemoryArenaBase* primArena_;
	PrimTypeNumArr primCounts_;
	PrimativeArry primitives_;
};


struct Layer
{
	typedef Flags<LayerFlag> LayerFlags;

	core::string name;
	LayerFlags flags;
};

} // namespace mapFile

X_NAMESPACE_END

#include "MapTypes.inl"

#endif // X_MAP_TYPES_H_