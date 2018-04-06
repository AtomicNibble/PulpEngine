#include "EngineCommon.h"
#include "XFrustum.h"

#include "Util\FloatIEEEUtil.h"

namespace Frustum
{
    uint8 BoxSides[0x40 * 8] = {
        0, 0, 0, 0, 0, 0, 0, 0, //00
        0, 4, 6, 2, 0, 0, 0, 4, //01
        7, 5, 1, 3, 0, 0, 0, 4, //02
        0, 0, 0, 0, 0, 0, 0, 0, //03
        0, 1, 5, 4, 0, 0, 0, 4, //04
        0, 1, 5, 4, 6, 2, 0, 6, //05
        7, 5, 4, 0, 1, 3, 0, 6, //06
        0, 0, 0, 0, 0, 0, 0, 0, //07
        7, 3, 2, 6, 0, 0, 0, 4, //08
        0, 4, 6, 7, 3, 2, 0, 6, //09
        7, 5, 1, 3, 2, 6, 0, 6, //0a
        0, 0, 0, 0, 0, 0, 0, 0, //0b
        0, 0, 0, 0, 0, 0, 0, 0, //0c
        0, 0, 0, 0, 0, 0, 0, 0, //0d
        0, 0, 0, 0, 0, 0, 0, 0, //0e
        0, 0, 0, 0, 0, 0, 0, 0, //0f
        0, 2, 3, 1, 0, 0, 0, 4, //10
        0, 4, 6, 2, 3, 1, 0, 6, //11
        7, 5, 1, 0, 2, 3, 0, 6, //12
        0, 0, 0, 0, 0, 0, 0, 0, //13
        0, 2, 3, 1, 5, 4, 0, 6, //14
        1, 5, 4, 6, 2, 3, 0, 6, //15
        7, 5, 4, 0, 2, 3, 0, 6, //16
        0, 0, 0, 0, 0, 0, 0, 0, //17
        0, 2, 6, 7, 3, 1, 0, 6, //18
        0, 4, 6, 7, 3, 1, 0, 6, //19
        7, 5, 1, 0, 2, 6, 0, 6, //1a
        0, 0, 0, 0, 0, 0, 0, 0, //1b
        0, 0, 0, 0, 0, 0, 0, 0, //1c
        0, 0, 0, 0, 0, 0, 0, 0, //1d
        0, 0, 0, 0, 0, 0, 0, 0, //1e
        0, 0, 0, 0, 0, 0, 0, 0, //1f
        7, 6, 4, 5, 0, 0, 0, 4, //20
        0, 4, 5, 7, 6, 2, 0, 6, //21
        7, 6, 4, 5, 1, 3, 0, 6, //22
        0, 0, 0, 0, 0, 0, 0, 0, //23
        7, 6, 4, 0, 1, 5, 0, 6, //24
        0, 1, 5, 7, 6, 2, 0, 6, //25
        7, 6, 4, 0, 1, 3, 0, 6, //26
        0, 0, 0, 0, 0, 0, 0, 0, //27
        7, 3, 2, 6, 4, 5, 0, 6, //28
        0, 4, 5, 7, 3, 2, 0, 6, //29
        6, 4, 5, 1, 3, 2, 0, 6, //2a
        0, 0, 0, 0, 0, 0, 0, 0, //2b
        0, 0, 0, 0, 0, 0, 0, 0, //2c
        0, 0, 0, 0, 0, 0, 0, 0, //2d
        0, 0, 0, 0, 0, 0, 0, 0, //2e
        0, 0, 0, 0, 0, 0, 0, 0, //2f
        0, 0, 0, 0, 0, 0, 0, 0, //30
        0, 0, 0, 0, 0, 0, 0, 0, //31
        0, 0, 0, 0, 0, 0, 0, 0, //32
        0, 0, 0, 0, 0, 0, 0, 0, //33
        0, 0, 0, 0, 0, 0, 0, 0, //34
        0, 0, 0, 0, 0, 0, 0, 0, //35
        0, 0, 0, 0, 0, 0, 0, 0, //36
        0, 0, 0, 0, 0, 0, 0, 0, //37
        0, 0, 0, 0, 0, 0, 0, 0, //38
        0, 0, 0, 0, 0, 0, 0, 0, //39
        0, 0, 0, 0, 0, 0, 0, 0, //3a
        0, 0, 0, 0, 0, 0, 0, 0, //3b
        0, 0, 0, 0, 0, 0, 0, 0, //3c
        0, 0, 0, 0, 0, 0, 0, 0, //3d
        0, 0, 0, 0, 0, 0, 0, 0, //3e
        0, 0, 0, 0, 0, 0, 0, 0, //3f
    };
}

void XFrustum::setFrustum(uint32_t nWidth, uint32_t nHeight, float32_t FOV, float32_t nearplane,
    float32_t farpane, float32_t fPixelAspectRatio)
{
    X_ASSERT(nearplane > 0.001f, "near plane not valid")
    (nearplane); //check if near-plane is valid
    X_ASSERT(farpane > 0.1f, "far plane not valid")
    (farpane); //check if far-plane is valid
    X_ASSERT(farpane > nearplane, "farplane must be greater than nearplane")
    (farpane, nearplane); //check if far-plane bigger then near-plane
    X_ASSERT(FOV >= 0.000001f && FOV < X_PI, "invalid FOV")
    (FOV); //check if specified FOV is valid

    fov_ = FOV;
    width_ = nWidth;   //surface x-resolution
    height_ = nHeight; //surface z-resolution
    near_ = nearplane;
    far_ = farpane;

    float32_t fWidth = (static_cast<float32_t>(nWidth) / fPixelAspectRatio);
    float32_t fHeight = static_cast<float32_t>(nHeight);

    projectionRatio_ = fWidth / fHeight; // projection ratio (1.0 for square pixels)
    pixelAspectRatio_ = fPixelAspectRatio;

    // calculate the Left/Top edge of the Projection-Plane in EYE-SPACE
    float projLeftTopX = -fWidth * 0.5f;
    float projLeftTopY = static_cast<float32_t>((1.0f / math<float>::tan(fov_ * 0.5f)) * (fHeight * 0.5f));
    float projLeftTopZ = fHeight * 0.5f;

    edge_plt_.x = projLeftTopX;
    edge_plt_.y = projLeftTopY;
    edge_plt_.z = projLeftTopZ;
    X_ASSERT(math<float>::abs(math<float>::acos(Vec3f(0, edge_plt_.y, edge_plt_.z).normalized().y) * 2 - fov_) < 0.001, "")
    ();

    // this is the left/upper vertex of the projection-plane (local-space)
    float invProjLeftTopY = 1.0f / projLeftTopY;
    edge_nlt_.x = nearplane * projLeftTopX * invProjLeftTopY;
    edge_nlt_.y = nearplane;
    edge_nlt_.z = nearplane * projLeftTopZ * invProjLeftTopY;

    //calculate the left/upper edge of the far-plane (=not rotated)
    edge_flt_.x = projLeftTopX * (farpane * invProjLeftTopY);
    edge_flt_.y = farpane;
    edge_flt_.z = projLeftTopZ * (farpane * invProjLeftTopY);

    UpdateFrustum();
}

void XFrustum::setFov(float32_t fov)
{
    X_ASSERT(fov >= 0.000001f && fov < X_PI, "invalid fov")
    (fov); //check if specified FOV is valid

    fov_ = fov;

    float32_t fWidth = (static_cast<float32_t>(width_) / pixelAspectRatio_);
    float32_t fHeight = static_cast<float32_t>(height_);

    // calculate the Left/Top edge of the Projection-Plane in EYE-SPACE
    float projLeftTopX = -fWidth * 0.5f;
    float projLeftTopY = static_cast<float32_t>((1.0f / math<float>::tan(fov_ * 0.5f)) * (fHeight * 0.5f));
    float projLeftTopZ = fHeight * 0.5f;

    edge_plt_.x = projLeftTopX;
    edge_plt_.y = projLeftTopY;
    edge_plt_.z = projLeftTopZ;
    X_ASSERT(math<float>::abs(math<float>::acos(Vec3f(0, edge_plt_.y, edge_plt_.z).normalized().y) * 2 - fov_) < 0.001, "")
    ();

    // this is the left/upper vertex of the projection-plane (local-space)
    float invProjLeftTopY = 1.0f / projLeftTopY;
    edge_nlt_.x = near_ * projLeftTopX * invProjLeftTopY;
    edge_nlt_.y = near_;
    edge_nlt_.z = near_ * projLeftTopZ * invProjLeftTopY;

    //calculate the left/upper edge of the far-plane (=not rotated)
    edge_flt_.x = projLeftTopX * (far_ * invProjLeftTopY);
    edge_flt_.y = far_;
    edge_flt_.z = projLeftTopZ * (far_ * invProjLeftTopY);

    UpdateFrustum();
}

void XFrustum::UpdateFrustum(void)
{
    // Local-space
    Matrix33f m33 = Matrix33f(mat_);

    proVerts[PlaneVert::TLEFT] = m33 * Vec3f(+edge_plt_.x, +edge_plt_.y, +edge_plt_.z);
    proVerts[PlaneVert::TRIGHT] = m33 * Vec3f(-edge_plt_.x, +edge_plt_.y, +edge_plt_.z);
    proVerts[PlaneVert::BLEFT] = m33 * Vec3f(+edge_plt_.x, +edge_plt_.y, -edge_plt_.z);
    proVerts[PlaneVert::BRIGHT] = m33 * Vec3f(-edge_plt_.x, +edge_plt_.y, -edge_plt_.z);

    npVerts[PlaneVert::TLEFT] = m33 * Vec3f(+edge_nlt_.x, +edge_nlt_.y, +edge_nlt_.z);
    npVerts[PlaneVert::TRIGHT] = m33 * Vec3f(-edge_nlt_.x, +edge_nlt_.y, +edge_nlt_.z);
    npVerts[PlaneVert::BLEFT] = m33 * Vec3f(+edge_nlt_.x, +edge_nlt_.y, -edge_nlt_.z);
    npVerts[PlaneVert::BRIGHT] = m33 * Vec3f(-edge_nlt_.x, +edge_nlt_.y, -edge_nlt_.z);

    fpVerts[PlaneVert::TLEFT] = m33 * Vec3f(+edge_flt_.x, +edge_flt_.y, +edge_flt_.z);
    fpVerts[PlaneVert::TRIGHT] = m33 * Vec3f(-edge_flt_.x, +edge_flt_.y, +edge_flt_.z);
    fpVerts[PlaneVert::BLEFT] = m33 * Vec3f(+edge_flt_.x, +edge_flt_.y, -edge_flt_.z);
    fpVerts[PlaneVert::BRIGHT] = m33 * Vec3f(-edge_flt_.x, +edge_flt_.y, -edge_flt_.z);

    //  calculate the six frustum-planes using the fustum edges in world-space
    Vec3f pos = getPosition();
    Vec3f crtn = npVerts[PlaneVert::TRIGHT];
    Vec3f cltn = npVerts[PlaneVert::TLEFT];
    Vec3f crbn = npVerts[PlaneVert::BRIGHT];

    Vec3f crbf = fpVerts[PlaneVert::BRIGHT];
    Vec3f crtf = fpVerts[PlaneVert::TRIGHT];
    Vec3f cltf = fpVerts[PlaneVert::TLEFT];
    Vec3f clbf = fpVerts[PlaneVert::BLEFT];

    planes_[FrustumPlane::NEAR] = Planef(crtn + pos, cltn + pos, crbn + pos);
    planes_[FrustumPlane::RIGHT] = Planef(crbf + pos, crtf + pos, pos);
    planes_[FrustumPlane::LEFT] = Planef(cltf + pos, clbf + pos, pos);
    planes_[FrustumPlane::TOP] = Planef(crtf + pos, cltf + pos, pos);
    planes_[FrustumPlane::BOTTOM] = Planef(clbf + pos, crbf + pos, pos);
    planes_[FrustumPlane::FAR] = Planef(crtf + pos, crbf + pos, cltf + pos); //clip-plane

    for (int i = 0; i < FrustumPlane::ENUM_COUNT; i++) {
        const Vec3f& normal = planes_[i].getNormal();

        uint32 bitX = core::FloatUtil::isSignBitNotSet(normal.x) ? 1 : 0;
        uint32 bitY = core::FloatUtil::isSignBitNotSet(normal.y) ? 1 : 0;
        uint32 bitZ = core::FloatUtil::isSignBitNotSet(normal.z) ? 1 : 0;

        idx_[i] = bitX * 3 + 0;
        idy_[i] = bitY * 3 + 1;
        idz_[i] = bitZ * 3 + 2;
    }
}

// --------------------------------

void XFrustum::GetFrustumVertices(FarNearVertsArr& verts) const
{
    Matrix33f m33 = Matrix33f(mat_);
    Vec3f pos = getPosition();

    verts[0] = m33 * Vec3f(+edge_flt_.x, +edge_flt_.y, +edge_flt_.z) + pos;
    verts[1] = m33 * Vec3f(+edge_flt_.x, +edge_flt_.y, -edge_flt_.z) + pos;
    verts[2] = m33 * Vec3f(-edge_flt_.x, +edge_flt_.y, -edge_flt_.z) + pos;
    verts[3] = m33 * Vec3f(-edge_flt_.x, +edge_flt_.y, +edge_flt_.z) + pos;

    verts[4] = m33 * Vec3f(+edge_nlt_.x, +edge_nlt_.y, +edge_nlt_.z) + pos;
    verts[5] = m33 * Vec3f(+edge_nlt_.x, +edge_nlt_.y, -edge_nlt_.z) + pos;
    verts[6] = m33 * Vec3f(-edge_nlt_.x, +edge_nlt_.y, -edge_nlt_.z) + pos;
    verts[7] = m33 * Vec3f(-edge_nlt_.x, +edge_nlt_.y, +edge_nlt_.z) + pos;
}

void XFrustum::GetFrustumVertices(FarProNearVertsArr& verts) const
{
    Matrix33f m33 = Matrix33f(mat_);
    Vec3f pos = getPosition();

    verts[0] = m33 * Vec3f(+edge_flt_.x, +edge_flt_.y, +edge_flt_.z) + pos;
    verts[1] = m33 * Vec3f(+edge_flt_.x, +edge_flt_.y, -edge_flt_.z) + pos;
    verts[2] = m33 * Vec3f(-edge_flt_.x, +edge_flt_.y, -edge_flt_.z) + pos;
    verts[3] = m33 * Vec3f(-edge_flt_.x, +edge_flt_.y, +edge_flt_.z) + pos;

    verts[4] = m33 * Vec3f(+edge_plt_.x, +edge_plt_.y, +edge_plt_.z) + pos;
    verts[5] = m33 * Vec3f(+edge_plt_.x, +edge_plt_.y, -edge_plt_.z) + pos;
    verts[6] = m33 * Vec3f(-edge_plt_.x, +edge_plt_.y, -edge_plt_.z) + pos;
    verts[7] = m33 * Vec3f(-edge_plt_.x, +edge_plt_.y, +edge_plt_.z) + pos;

    verts[8] = m33 * Vec3f(+edge_nlt_.x, +edge_nlt_.y, +edge_nlt_.z) + pos;
    verts[9] = m33 * Vec3f(+edge_nlt_.x, +edge_nlt_.y, -edge_nlt_.z) + pos;
    verts[10] = m33 * Vec3f(-edge_nlt_.x, +edge_nlt_.y, -edge_nlt_.z) + pos;
    verts[11] = m33 * Vec3f(-edge_nlt_.x, +edge_nlt_.y, +edge_nlt_.z) + pos;
}

// --------------------------------

void XFrustum::getNearPlaneCoordinates(Vec3f* pTopLeft, Vec3f* pTopRight,
    Vec3f* pBottomLeft, Vec3f* pBottomRight) const
{
    *pTopLeft = npVerts[PlaneVert::TLEFT];
    *pTopRight = npVerts[PlaneVert::TRIGHT];
    *pBottomLeft = npVerts[PlaneVert::BLEFT];
    *pBottomRight = npVerts[PlaneVert::BRIGHT];
}

void XFrustum::getProPlaneCoordinates(Vec3f* pTopLeft, Vec3f* pTopRight,
    Vec3f* pBottomLeft, Vec3f* pBottomRight) const
{
    *pTopLeft = proVerts[PlaneVert::TLEFT];
    *pTopRight = proVerts[PlaneVert::TRIGHT];
    *pBottomLeft = proVerts[PlaneVert::BLEFT];
    *pBottomRight = proVerts[PlaneVert::BRIGHT];
}

void XFrustum::getFarPlaneCoordinates(Vec3f* pTopLeft, Vec3f* pTopRight,
    Vec3f* pBottomLeft, Vec3f* pBottomRight) const
{
    *pTopLeft = fpVerts[PlaneVert::TLEFT];
    *pTopRight = fpVerts[PlaneVert::TRIGHT];
    *pBottomLeft = fpVerts[PlaneVert::BLEFT];
    *pBottomRight = fpVerts[PlaneVert::BRIGHT];
}
