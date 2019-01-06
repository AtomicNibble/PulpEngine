#pragma once

struct Vertex_P3F_T2F_C4B;

#include <IRender.h>
#include <Math\XRect.h>

#include <Traits\FunctionTraits.h>

X_NAMESPACE_DECLARE(font, struct TextDrawContext);

X_NAMESPACE_BEGIN(engine)

class Material;

class IPrimativeContext
{
public:
    typedef render::TopoType PrimitiveType;

    enum class Mode
    {
        Mode2D,
        Mode3D,
    };

    // Lod0 = highest quality.
    static const int32_t SHAPE_NUM_LOD = 4;

    X_DECLARE_ENUM(ShapeType)
    (
        Sphere,
        Cone,
        Cylinder // get fooked. AINT nobody got time for cylinders.
    );

    // typedef InstancedData_MAT44_C4F ShapeInstanceData;
    X_ALIGNED_SYMBOL(struct ShapeInstanceData, 16) :
        public InstancedData_MAT44_C4F
    {
        // not much point making a move assign op.
        X_INLINE ShapeInstanceData& operator=(const ShapeInstanceData& oth)
        {
            mat = oth.mat;
            color = oth.color;
            lodIdx = oth.lodIdx;
            solid = oth.solid;
            return *this;
        }

        int32_t lodIdx;
        int32_t solid;
        uint8_t _pad[4];
    };

public:
    typedef Vertex_P3F_T2S_C4B PrimVertex;
    static const auto VERTEX_FMT = render::shader::VertexFormat::P3F_T2S_C4B;

    typedef std::array<float, 4> CoordArr;

protected:
    IPrimativeContext();

public:
    virtual ~IPrimativeContext();

    virtual size_t maxVertsPerPrim(void) const X_ABSTRACT;
    virtual Mode getMode(void) const X_ABSTRACT;
    virtual void reset(void) X_ABSTRACT;
    virtual void setDepthTest(bool enabled) X_ABSTRACT;

    X_INLINE void drawQuad(float x, float y, float width, float height, Material* pMaterial, Color8u col);
    X_INLINE void drawQuad(const Rectf& rect, Material* pMaterial, Color8u col);
    X_INLINE void drawQuad(const Rectf& rect, Color8u col);

    // for 2d, z is depth not position
    void drawQuad(float x, float y, float z, float width, float height, Color8u col);
    X_INLINE void drawQuad(float x, float y, float z, float width, float height, Color8u col, Color8u borderCol);
    X_INLINE void drawQuad(float x, float y, float width, float height, Color8u col);
    X_INLINE void drawQuad(float x, float y, float width, float height, Color8u col, Color8u borderCol);
    X_INLINE void drawQuad(Vec2<float> pos, float width, float height, Color8u col);
    X_INLINE void drawQuad(Vec2<float> pos, Vec2<float> size, Color8u col);

    // draw a quad in 3d z is position not depth.
    void drawQuad(const Vec3f& tl, const Vec3f& tr, const Vec3f& bl, const Vec3f& br, Color8u col);
    void drawQuad(const Vec3f& tl, const Vec3f& tr, const Vec3f& bl, const Vec3f& br, Material* pMaterial, Color8u col);
    void drawQuad(const Vec3f& tl, const Vec3f& tr, const Vec3f& bl, const Vec3f& br, Material* pMaterial, Color8u col, const Rectf& r);

    void drawLines(core::span<const Vec3f> points, Color8u col);
    void drawLines(core::span<const Vec3f> points, core::span<const Color8u> col);

    X_INLINE void drawLine(const Vec3f& pos1, const Vec3f& pos2);
    X_INLINE void drawLine(const Vec3f& pos1, Color8u color1,
        const Vec3f& pos2, Color8u color2);
    X_INLINE void drawLine(const Vec3f& pos1, const Vec3f& pos2, Color8u color1);

    X_INLINE void drawRect(const Rectf& rect, Color8u col);
    X_INLINE void drawRect(const Vec2f& tl, const Vec2f& size, Color8u col);
    void drawRect(float x, float y, float width, float height, Color8u col);
    void drawRect(const Vec3f& tl, const Vec3f& tr, const Vec3f& bl, const Vec3f& br, Color8u col);
    void drawRect(const Vec3f& tl, const Vec3f& tr, const Vec3f& bl, const Vec3f& br,
        Color8u tlCol, Color8u trCol, Color8u blCol, Color8u brCol);

    // ya fucking what!
    void drawBarChart(const Rectf& rect, uint32_t num, const float* pHeights,
        float padding, uint32_t max, Color8u col);

    typedef core::traits::Function<float(const void* pData, int32_t idx)> DataCallBack;

    void drawGraph(const Rectf& rect, float* pBegin, float* pEnd, Color8u col, Color8u colBck,
        float scaleMin = std::numeric_limits<float>::max(), float scaleMax = std::numeric_limits<float>::lowest());
    void drawGraph(const Rectf& rect, const void* pUserData, size_t numValues, DataCallBack::Pointer dataFunc, Color8u col, Color8u colBck,
        float scaleMin = std::numeric_limits<float>::max(), float scaleMax = std::numeric_limits<float>::lowest());


    // Points
    X_INLINE void drawPoint(const Vec3f& pos, Color8u col, uint8_t size = 1);
    X_INLINE void drawPoints(const Vec3f* pPoints, uint32_t numPoints, Color8u col, uint8_t size = 1);
    X_INLINE void drawPoints(const Vec3f* pPoints, uint32_t numPoints, const Color8u* pCol, uint8_t size = 1);

    // Triangle
    X_INLINE void drawTriangle(const Vec3f& v0, Color8u col0,
        const Vec3f& v1, Color8u col1,
        const Vec3f& v2, Color8u col2);
    X_INLINE void drawTriangle(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, Color8u col);

    void drawTriangle(const Vec3f* pPoints, size_t numPoints, Color8u c0);
    void drawTriangle(const Vec3f* pPoints, size_t numPoints, const Color8u* pCol);

    // AABB
    void drawAABB(const AABB& aabb, bool solid, Color8u col);

    // OBB
    void drawOBB(const OBB& obb, bool solid, Color8u col);
    void drawOBB(const OBB& obb, const Vec3f& offset, bool solid, Color8u col);

    // Sphere
    void drawSphere(const Sphere& sphere, Color8u col, bool solid = true, int32_t lodIdx = 0);
    void drawSphere(const Sphere& sphere, const Matrix34f& mat, Color8u col, bool solid = true, int32_t lodIdx = 0);

    // Cone
    void drawCone(const Vec3f& pos, const Vec3f& dir, float radius, float height, Color8u col, bool solid = true, int32_t lodIdx = 0);

    // Cylinder
    void drawCylinder(const Vec3f& pos, const Vec3f& dir, float radius, float height, Color8u col, bool solid = true, int32_t lodIdx = 0);

    // Capsule
    void drawCapsule(const Transformf& trans, float radius, float height, Color8u col);

    // Circle
    void drawCircle(const Transformf& trans, int32_t segments, float radius, Color8u col);

    // Arc
    void drawArc(const Transformf& trans, int32_t segments, float radius, float minAng, float maxAng, Color8u col);

    // Bone
    void drawBone(const Transformf& rParent, const Transformf& rBone, Color8u col);
    X_INLINE void drawBone(const Matrix44f& rParent, const Matrix44f& rBone, Color8u col);

    // Frustum - Sexyyyyyyyyyy
    void drawFrustum(const XFrustum& frustum, Color8u nearCol, Color8u farCol, bool solid = false);

    // Arrow
    void drawArrow(const Vec3f& posA, const Vec3f& posB, Color8u color);

    // Axis - X: red Y: Green Z: Blue
    void drawAxis(const Vec3f& pos, const Vec3f& extends);
    void drawAxis(const Transformf& trans, const Vec3f& extends);
    void drawAxis(const Transformf& trans, const Vec3f& offset, const Vec3f& extends);

    // CrosssHair
    void drawCrosshair(const Vec3f& pos, float size, Color8u color);

    // format buffer is 2048 in size.
    X_INLINE void drawText(const Vec3f& pos, const font::TextDrawContext& con, const char* pFormat, va_list args);
    X_INLINE void drawText(const Vec3f& pos, const font::TextDrawContext& con, const char* pText);
    X_INLINE void drawText(const Vec3f& pos, const Matrix33f& ang, const font::TextDrawContext& con, const char* pText);
    X_INLINE void drawText(float x, float y, const font::TextDrawContext& con, const char* pText);
    X_INLINE void drawText(float x, float y, const font::TextDrawContext& con, const char* pText, const char* pEnd);

    X_INLINE void drawText(const Vec3f& pos, const font::TextDrawContext& con, const wchar_t* pFormat, va_list args);
    X_INLINE void drawText(const Vec3f& pos, const font::TextDrawContext& con, const wchar_t* pText);
    X_INLINE void drawText(const Vec3f& pos, const Matrix33f& ang, const font::TextDrawContext& con, const wchar_t* pText);
    X_INLINE void drawText(float x, float y, const font::TextDrawContext& con, const wchar_t* pText);
    X_INLINE void drawText(float x, float y, const font::TextDrawContext& con, const wchar_t* pText, const wchar_t* pEnd);

    virtual void drawText(const Vec3f& poss, const Matrix33f& ang, const font::TextDrawContext& con, const char* pText, const char* pEnd) X_ABSTRACT;
    virtual void drawText(const Vec3f& poss, const Matrix33f& ang, const font::TextDrawContext& con, const wchar_t* pText, const wchar_t* pEnd) X_ABSTRACT;
    virtual void drawText(const Vec3f& poss, const font::TextDrawContext& con, const char* pText, const char* pEnd) X_ABSTRACT;
    virtual void drawText(const Vec3f& poss, const font::TextDrawContext& con, const wchar_t* pText, const wchar_t* pEnd) X_ABSTRACT;

private:
    X_INLINE void drawImage(float xpos, float ypos, float z, float w, float h,
        Material* pMaterial, float s0, float t0, float s1, float t1, const Colorf& col, bool filtered = true);

    void drawImageWithUV(float xpos, float ypos, float z, float w, float h,
        Material* pMaterial, const CoordArr& s, const CoordArr& t, const Colorf& col, bool filtered = true);

public:
    virtual PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type, Material* pMaterial) X_ABSTRACT;
    virtual PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type) X_ABSTRACT;

    virtual ShapeInstanceData* addShape(ShapeType::Enum type, bool solid, int32_t lodIdx = 0) X_ABSTRACT;
};

X_NAMESPACE_END

#include "IPrimativeContext.inl"
