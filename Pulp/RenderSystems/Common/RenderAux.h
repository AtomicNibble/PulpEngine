#pragma once

#ifndef X_COMMON_RENDER_AUX_H_
#define X_COMMON_RENDER_AUX_H_

#include <IRenderAux.h>

#include <Math\VertexFormats.h>

#include <vector>

X_NAMESPACE_BEGIN(render)

class XRenderAux;
struct XAuxGeomCBRawDataPackaged;

struct IRenderAuxImpl
{
public:
    virtual ~IRenderAuxImpl()
    {
    }
    virtual void Flush(const XAuxGeomCBRawDataPackaged& data, size_t begin, size_t end) X_ABSTRACT;
    virtual void RT_Flush(const XAuxGeomCBRawDataPackaged& data, size_t begin, size_t end) X_ABSTRACT;
};

class XRenderAux : public IRenderAux
{
public:
    XRenderAux(IRenderAuxImpl* pRenderAugImp);
    ~XRenderAux() X_OVERRIDE;

    void flush(void) X_OVERRIDE;

    virtual void setRenderFlags(const XAuxGeomRenderFlags& renderFlags) X_OVERRIDE;
    virtual XAuxGeomRenderFlags getRenderFlags() X_OVERRIDE;

    // Lines
    void drawLine(const Vec3f& v0, const Color8u& c0, const Vec3f& v1, const Color8u& c1, float thickness = 1.f) X_OVERRIDE;

    void drawLines(Vec3f* points, uint32_t numPoints, const Color8u& col, float thickness = 1.f) X_OVERRIDE;
    void drawLines(Vec3f* points, uint32_t numPoints, Color8u* col, float thickness = 1.f) X_OVERRIDE;

    void drawLines(Vec3f* points, uint32_t numPoints, uint16_t* indices,
        uint32_t numIndices, const Color8u& col, float thickness = 1.f) X_OVERRIDE;
    void drawLines(Vec3f* points, uint32_t numPoints, uint16_t* indices,
        uint32_t numIndices, Color8u* col, float thickness = 1.f) X_OVERRIDE;

    // Triangles
    void drawTriangle(const Vec3f& v0, const Color8u& c0, const Vec3f& v1, const Color8u& c1,
        const Vec3f& v2, const Color8u& c2) X_OVERRIDE;

    void drawTriangle(const Vec3f* points, uint32_t numPoints, const Color8u& c0) X_OVERRIDE;
    void drawTriangle(const Vec3f* points, uint32_t numPoints, const Color8u* pCol) X_OVERRIDE;

    void drawTriangle(const Vec3f* points, uint32_t numPoints, const uint16_t* indices,
        uint32_t numIndices, const Color8u& c0) X_OVERRIDE;
    void drawTriangle(const Vec3f* points, uint32_t numPoints, const uint16_t* indices,
        uint32_t numIndices, const Color8u* pCol) X_OVERRIDE;

    // AABB
    void drawAABB(const AABB& aabb, bool solid, const Color8u& col) X_OVERRIDE;
    void drawAABB(const AABB& aabb, const Vec3f& pos, bool solid, const Color8u& col) X_OVERRIDE;
    void drawAABB(const AABB& aabb, const Matrix34f& mat, bool solid, const Color8u& col) X_OVERRIDE;

    // OBB
    void drawOBB(const OBB& obb, const Vec3f& pos, bool solid, const Color8u& col) X_OVERRIDE;
    void drawOBB(const OBB& obb, const Matrix34f& mat, bool solid, const Color8u& col) X_OVERRIDE;

    // Sphere
    void drawSphere(const Sphere& sphere, const Color8u& col, bool drawShaded = true) X_OVERRIDE;
    void drawSphere(const Sphere& sphere, const Matrix34f& mat, const Color8u& col, bool drawShaded = true) X_OVERRIDE;

    // Cone
    void drawCone(const Vec3f& pos, const Vec3f& dir, float radius, float height, const Color8u& col, bool drawShaded = true) X_OVERRIDE;

    // Cylinder
    void drawCylinder(const Vec3f& pos, const Vec3f& dir, float radius, float height, const Color8u& col, bool drawShaded = true) X_OVERRIDE;

    // Bone
    void drawBone(const Transformf& rParent, const Transformf& rBone, const Color8u& col) X_OVERRIDE;
    void drawBone(const Matrix34f& rParent, const Matrix34f& rBone, const Color8u& col) X_OVERRIDE;

    // Frustum
    void drawFrustum(const XFrustum& frustum, const Color8u& nearCol, const Color8u& farCol, bool drawShaded = false) X_FINAL;

public:
    X_DECLARE_ENUM(DrawObjType)
    (Sphere, Cone, Cylinder);
    X_DECLARE_ENUM(PrimType)
    (Invalid, PtList, LineList, LineListInd, TriList, TriListInd, Obj);

    struct AuxGeomPrivateBitMasks
    {
        // public field starts at bit 22
        enum Enum
        {
            PrimTypeShift = 19,
            PrimTypeMask = 0x7 << PrimTypeShift,

            PrivateRenderflagsMask = (1 << 19) - 1
        };
    };

    struct AuxGeomPrivateRenderflags
    {
        // for non-indexed triangles
        enum Enum
        {
            TriListParam_ProcessThickLines = 0x00000001,

        };
    };

    struct XAuxDrawObjParams
    {
        XAuxDrawObjParams()
        {
        }
        XAuxDrawObjParams(const Matrix34f& matWorld, const Color8u& color,
            float size, bool shaded) :
            matWorld(matWorld),
            color(color),
            size(size),
            shaded(shaded)
        {
        }

        Matrix34f matWorld;
        Color8u color;
        float size;
        bool shaded;
    };

    struct XAuxPushBufferEntry
    {
        XAuxPushBufferEntry()
        {
        }

        XAuxPushBufferEntry(uint32 numVertices, uint32 numIndices,
            uint32 vertexOffs, uint32 indexOffs, uint32 transMatrixIdx,
            const XAuxGeomRenderFlags& renderFlags) :
            numVertices(numVertices),
            numIndices(numIndices),
            vertexOffs(vertexOffs),
            indexOffs(indexOffs),
            transMatrixIdx(transMatrixIdx),
            renderFlags(renderFlags)
        {
        }

        XAuxPushBufferEntry(uint32 drawParamOffs, uint32 transMatrixIdx,
            const XAuxGeomRenderFlags& renderFlags) :
            numVertices(0),
            numIndices(0),
            vertexOffs(drawParamOffs),
            indexOffs(0),
            transMatrixIdx(transMatrixIdx),
            renderFlags(renderFlags)
        {
            X_ASSERT(PrimType::Obj == GetPrimType(renderFlags), "invalid primative type")
            (GetPrimType(renderFlags));
        }

        bool GetDrawParamOffs(uint32& drawParamOffs) const
        {
            if (PrimType::Obj == GetPrimType(renderFlags)) {
                drawParamOffs = vertexOffs;
                return true;
            }
            return false;
        }

        uint32 numVertices;
        uint32 numIndices;
        uint32 vertexOffs;
        uint32 indexOffs;
        int transMatrixIdx;
        XAuxGeomRenderFlags renderFlags;
    };

    typedef std::vector<XAuxPushBufferEntry> AuxPushBuffer;
    typedef std::vector<const XAuxPushBufferEntry*> AuxSortedPushBuffer;
    typedef std::vector<XAuxVertex> AuxVertexBuffer;
    typedef std::vector<uint16_t> AuxIndexBuffer;
    typedef std::vector<XAuxDrawObjParams> AuxDrawObjParamBuffer;
    typedef std::vector<Matrix44f> AuxOrthoMatrixBuffer;

    struct XAuxGeomCBRawData
    {
    public:
        void GetSortedPushBuffer(size_t begin, size_t end,
            AuxSortedPushBuffer& auxSortedPushBuffer) const;

    public:
        AuxPushBuffer auxPushBuffer;
        AuxVertexBuffer auxVertexBuffer;
        AuxIndexBuffer auxIndexBuffer;
        AuxDrawObjParamBuffer auxDrawObjParamBuffer;
        AuxOrthoMatrixBuffer auxOrthoMatrices;
    };

    static PrimType::Enum GetPrimType(const XAuxGeomRenderFlags& renderFlags);
    static bool IsThickLine(const XAuxGeomRenderFlags& renderFlags);
    static DrawObjType::Enum GetAuxObjType(const XAuxGeomRenderFlags& renderFlags);
    static uint8 GetPointSize(const XAuxGeomRenderFlags& renderFlags);

    void Reset(void)
    {
        data_.Reset();
    }

    void FreeMemory()
    {
        data_.FreeMemory();
    }

    // setting orthogonal projection
    void SetOrthoMode(bool enable, Matrix44f* pMatrix = nullptr)
    {
        data_.SetOrthoMode(enable, pMatrix);
    }

private:
    uint32 CreatePointRenderFlags(uint8 size);
    uint32 CreateLineRenderFlags(bool indexed);
    uint32 CreateTriangleRenderFlags(bool indexed);
    uint32 CreateObjectRenderFlags(const DrawObjType::Enum objType);

    void DrawThickLine(const Vec3f& v0, const Color8u& col0,
        const Vec3f& v1, const Color8u& col1, float thickness);

    void AddPushBufferEntry(uint32 numVertices, uint32 numIndices,
        const XAuxGeomRenderFlags& renderFlags);

    void AddPrimitive(XAuxVertex*& pVertices, uint32 numVertices,
        const XAuxGeomRenderFlags& renderFlags);
    void AddIndexedPrimitive(XAuxVertex*& pVertices, uint32 numVertices,
        uint16*& pIndices, uint32 numIndices, const XAuxGeomRenderFlags& renderFlags);

    void AddObject(XAuxDrawObjParams*& pDrawParams, const XAuxGeomRenderFlags& renderFlags);

    struct PushBufferSortFunc
    {
        bool operator()(const XAuxPushBufferEntry* lhs, const XAuxPushBufferEntry* rhs) const
        {
            if (lhs->renderFlags != rhs->renderFlags)
                return lhs->renderFlags < rhs->renderFlags;
            return lhs->transMatrixIdx < rhs->transMatrixIdx;
        }
    };

    class XRawCBDataContainer
    {
    public:
        XRawCBDataContainer() :
            curTransMatIdx_(-1),
            lastFlushPos_(0)
        {
        }

        XAuxGeomCBRawData& Access()
        {
            return cbData_;
        }

        const XAuxGeomCBRawData& Access() const
        {
            return cbData_;
        }

        size_t GetLastFlushPos() const
        {
            return lastFlushPos_;
        }

        size_t GetCurFlushPos() const
        {
            return Access().auxPushBuffer.size();
        }

        void UpdateLastFlushPos()
        {
            lastFlushPos_ = GetCurFlushPos();
        }

        void Reset()
        {
            curTransMatIdx_ = -1;
            lastFlushPos_ = 0;

            cbData_.auxPushBuffer.clear();
            cbData_.auxVertexBuffer.clear();
            cbData_.auxIndexBuffer.clear();
            cbData_.auxDrawObjParamBuffer.clear();
            cbData_.auxOrthoMatrices.clear();
        }

        void FreeMemory()
        {
            curTransMatIdx_ = -1;
            lastFlushPos_ = 0;

            //	for (size_t i = 0, c = sizeof(m_cbData) / sizeof(m_cbData[0]); i != c; ++i)
            //		stl::reconstruct(m_cbData[i]);
        }

        int GetTransMatIdx() const
        {
            return curTransMatIdx_;
        }

        void SetOrthoMode(bool enable, Matrix44f* pMatrix = nullptr)
        {
            if (enable) {
                X_ASSERT_NOT_NULL(pMatrix);
                curTransMatIdx_ = safe_static_cast<int, size_t>(Access().auxOrthoMatrices.size());
                Access().auxOrthoMatrices.push_back(*pMatrix);
            }
            else
                curTransMatIdx_ = -1;
        }

    private:
        int curTransMatIdx_;
        size_t lastFlushPos_;
        XAuxGeomCBRawData cbData_;
    };

    int GetTransMatrixIndex()
    {
        return data_.GetTransMatIdx();
    }

    XAuxGeomCBRawData& AccessData()
    {
        return data_.Access();
    }

private:
    XAuxGeomRenderFlags curRenderFlags_;
    IRenderAuxImpl* pRenderAuxGeom_;
    XRawCBDataContainer data_;
};

inline uint32 XRenderAux::CreateLineRenderFlags(bool indexed)
{
    if (false != indexed) {
        return (curRenderFlags_ | (PrimType::LineListInd << AuxGeomPrivateBitMasks::PrimTypeShift));
    }

    return (curRenderFlags_ | (PrimType::LineList << AuxGeomPrivateBitMasks::PrimTypeShift));
}

inline uint32 XRenderAux::CreateTriangleRenderFlags(bool indexed)
{
    if (false != indexed) {
        return (curRenderFlags_ | (PrimType::TriListInd << AuxGeomPrivateBitMasks::PrimTypeShift));
    }

    return (curRenderFlags_ | (PrimType::TriList << AuxGeomPrivateBitMasks::PrimTypeShift));
}

inline uint32 XRenderAux::CreateObjectRenderFlags(const DrawObjType::Enum objType)
{
    return (curRenderFlags_ | (PrimType::Obj << AuxGeomPrivateBitMasks::PrimTypeShift) | objType);
}

inline XRenderAux::PrimType::Enum XRenderAux::GetPrimType(const XAuxGeomRenderFlags& renderFlags)
{
    uint32 primType((renderFlags & AuxGeomPrivateBitMasks::PrimTypeMask) >> AuxGeomPrivateBitMasks::PrimTypeShift);
    switch (primType) {
        case PrimType::PtList:
        case PrimType::LineList:
        case PrimType::LineListInd:
        case PrimType::TriList:
        case PrimType::TriListInd:
        case PrimType::Obj:
            return (PrimType::Enum)primType;
        default: {
            X_ASSERT_UNREACHABLE();
        }
    }

    return PrimType::Obj;
}

inline bool XRenderAux::IsThickLine(const XAuxGeomRenderFlags& renderFlags)
{
    PrimType::Enum primType(GetPrimType(renderFlags));
    X_ASSERT(PrimType::TriList == primType, "invalid primtype")
    (primType);

    if (PrimType::TriList == primType) {
        return (0 != (renderFlags & AuxGeomPrivateRenderflags::TriListParam_ProcessThickLines));
    }

    return false;
}

inline XRenderAux::DrawObjType::Enum XRenderAux::GetAuxObjType(const XAuxGeomRenderFlags& renderFlags)
{
    PrimType::Enum primType(GetPrimType(renderFlags));
    X_ASSERT(PrimType::Obj == primType, "invalid primtype")
    (primType);

    uint32 objType((renderFlags & AuxGeomPrivateBitMasks::PrivateRenderflagsMask));

    switch (objType) {
        case DrawObjType::Sphere:
        case DrawObjType::Cone:
        case DrawObjType::Cylinder:
            return (DrawObjType::Enum)objType;

        default: {
            X_ASSERT_UNREACHABLE();
        }
    }

    return (DrawObjType::Enum)objType;
}

inline uint8 XRenderAux::GetPointSize(const XAuxGeomRenderFlags& renderFlags)
{
    PrimType::Enum primType(GetPrimType(renderFlags));
    X_ASSERT(PrimType::PtList == primType, "invalid primtype")
    (primType);

    if (PrimType::PtList == primType) {
        // this is not correct.
        X_ASSERT_NOT_IMPLEMENTED();
        // return (renderFlags & AuxGeomPrivateBitMasks::PrivateRenderflagsMask);
    }

    return 0;
}

// package XRenderAux::XAuxGeomCBRawData ptr via seperate struct
// as nested types cannot be forward declared
struct XAuxGeomCBRawDataPackaged
{
    XAuxGeomCBRawDataPackaged(const XRenderAux::XAuxGeomCBRawData* pData) :
        pData_(pData)
    {
        X_ASSERT_NOT_NULL(pData_);
    }

    const XRenderAux::XAuxGeomCBRawData* pData_;
};

X_NAMESPACE_END

#endif // !X_COMMON_RENDER_AUX_H_
