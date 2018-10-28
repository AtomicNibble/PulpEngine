#pragma once

#ifndef X_MAP_TYPES_H_
#define X_MAP_TYPES_H_

#ifdef IGNORE
#undef IGNORE
#endif // !IGNORE

X_NAMESPACE_DECLARE(core,
                    struct XFile;)

X_NAMESPACE_BEGIN(level)

namespace mapFile
{
    X_DECLARE_ENUM(PrimType)
    (BRUSH, PATCH);
    X_DECLARE_FLAGS(LayerFlag)
    (ACTIVE, EXPANDED, IGNORE);

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

            MaterialName name;
            Vec2f matRepeate;
            Vec2f shift;
            float rotate;
            float scale;
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
        MaterialInfo material_;
        MaterialInfo lightMap_;
        Planef plane_;
    };

    class XMapBrush : public XMapPrimitive
    {
        typedef core::Array<XMapBrushSide*> BrushSidePtrArr;

    public:
        X_INLINE XMapBrush(core::MemoryArenaBase* arena, core::MemoryArenaBase* primArena);
        X_INLINE ~XMapBrush(void) X_OVERRIDE;

        X_INLINE size_t GetNumSides(void) const;
        X_INLINE void AddSide(XMapBrushSide* pSide);
        X_INLINE XMapBrushSide* GetSide(size_t i) const;

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
            int32_t verts[2]; // edge vertices always with ( verts[0] < verts[1] )
            int32_t tris[2];  // edge triangles
        };

        typedef core::Array<LvlVert> VertArr;
        typedef core::Array<int> IntArr;
        typedef core::Array<SurfaceEdge> SurfaceEdgeArr;

    public:
        XMapPatch(core::MemoryArenaBase* arena);
        XMapPatch(core::MemoryArenaBase* arena, int w, int h);
        ~XMapPatch(void) X_OVERRIDE;

        X_INLINE void SetHorzSubdivisions(int32_t num);
        X_INLINE void SetVertSubdivisions(int32_t num);
        X_INLINE int32_t GetHorzSubdivisions(void) const;
        X_INLINE int32_t GetVertSubdivisions(void) const;

        X_INLINE int32_t GetNumIndexes(void) const;
        X_INLINE const int* GetIndexes(void) const;

        X_INLINE const LvlVert& operator[](const int idx) const;
        X_INLINE LvlVert& operator[](const int idx);

        X_INLINE void SetMesh(bool b);
        X_INLINE const bool isMesh(void) const;
        X_INLINE const char* GetMatName(void) const;

        // Subdived util.
        void Subdivide(float maxHorizontalError, float maxVerticalError,
            float maxLength, bool genNormals = false);
        void SubdivideExplicit(int32_t horzSubdivisions, int32_t vertSubdivisions,
            bool genNormals, bool removeLinear = false);

        void CreateNormalsAndIndexes(void);

        bool Parse(core::XLexer& src, const Vec3f& origin);

    private:
        void PutOnCurve(void);
        void RemoveLinearColumnsRows(void);
        void ResizeExpanded(int32_t height, int32_t width);
        void Expand(void);
        void Collapse(void);
        void GenerateNormals(void);
        void GenerateIndexes(void);
        void LerpVert(const LvlVert& a, const LvlVert& b, LvlVert& out) const;
        void GenerateEdgeIndexes(void);

        void ProjectPointOntoVector(const Vec3f& point, const Vec3f& vStart,
            const Vec3f& vEnd, Vec3f& vProj);
        void SampleSinglePatch(const LvlVert ctrl[3][3], int32_t baseCol, int32_t baseRow,
            int32_t width, int32_t horzSub, int32_t vertSub, LvlVert* outVerts) const;
        void SampleSinglePatchPoint(const LvlVert ctrl[3][3], float u,
            float v, LvlVert* out) const;

    protected:
        VertArr verts_;
        IntArr indexes_;       // 3 references to vertices for each triangle
        SurfaceEdgeArr edges_; // edges
        IntArr edgeIndexes_;

        core::string matName_;
        core::string lightMap_;

        int32_t width_;
        int32_t height_;
        int32_t maxWidth_;
        int32_t maxHeight_;
        int32_t horzSubdivisions_;
        int32_t vertSubdivisions_;

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
        typedef std::array<size_t, PrimType::ENUM_COUNT> PrimTypeNumArr;

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