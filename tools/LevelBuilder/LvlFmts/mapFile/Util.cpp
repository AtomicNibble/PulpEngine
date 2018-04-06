#include "stdafx.h"
#include "Util.h"

X_NAMESPACE_BEGIN(level)

namespace mapFile
{
    namespace
    {
        const Vec3f baseaxis[18] = {
            {1.0, 0.0, 0.0},
            {0.0, -1.0, 0.0},
            {0.0, 0.0, -1.0},

            {1.0, 0.0, 0.0},
            {0.0, -1.0, 0.0},
            {1.0, 0.0, 0.0},

            {0.0, 1.0, 0.0},
            {0.0, 0.0, -1.0},
            {-1.0, 0.0, 0.0},

            {0.0, 1.0, 0.0},
            {0.0, 0.0, -1.0},
            {0.0, 1.0, 0.0},

            {1.0, 0.0, 0.0},
            {0.0, 0.0, -1.0},
            {0.0, -1.0, 0.0},

            {1.0, 0.0, 0.0},
            {0.0, 0.0, -1.0},
            {0.0, -1.0, 0.0}};
    }

    X_DISABLE_WARNING(4244)
    void TextureAxisFromPlane(const Vec3f& normal, Vec3f& a2, Vec3f& a3)
    {
        size_t axis = 0;
        float largestAxis = 0.0;

        const float x = normal[0];
        const float y = normal[1];
        const float z = normal[2];

        const float negX = -normal[0];
        const float negY = -normal[1];
        const float negZ = -normal[2];

        // Z
        if (z > 0.0) {
            largestAxis = z;
            axis = 0; // positive Z
        }

        if (largestAxis < negZ) {
            largestAxis = negZ;
            axis = 1; // negative Z
        }

        // X
        if (largestAxis < x) {
            largestAxis = x;
            axis = 2; // positive X
        }

        if (largestAxis < negX) {
            largestAxis = negX;
            axis = 3; // negative Z
        }

        // Y
        if (largestAxis < y) {
            largestAxis = y;
            axis = 4; // positive Y
        }

        if (largestAxis < negY) {
            axis = 5; // negative Y
        }

        X_ASSERT(((3 * axis + 1) < 18), "axis out of range")(axis); 
        a2 = baseaxis[3 * axis];
        a3 = baseaxis[3 * axis + 1];
    }

    X_ENABLE_WARNING(4244)

    void QuakeTextureVecs(const Planef& plane, Vec2f shift, float rotate, Vec2f scale, Vec4f mappingVecs[2])
    {
        Vec3f vecs[2];
        TextureAxisFromPlane(plane.getNormal(), vecs[0], vecs[1]);

        if (!scale[0]) {
            scale[0] = 1.f;
        }
        if (!scale[1]) {
            scale[1] = 1.f;
        }

        // rotate axis
        float ang, sinv, cosv;
        if (rotate == 0.f) {
            sinv = 0.f;
            cosv = 1.f;
        }
        else if (rotate == 90.f) {
            sinv = 1.f;
            cosv = 0.f;
        }
        else if (rotate == 180.f) {
            sinv = 0.f;
            cosv = -1.f;
        }
        else if (rotate == 270.f) {
            sinv = -1.f;
            cosv = 0.f;
        }
        else {
            ang = ::toRadians(rotate);
            sinv = sin(ang);
            cosv = cos(ang);
        }

        int32_t sv, tv;
        if (vecs[0][0]) {
            sv = 0;
        }
        else if (vecs[0][1]) {
            sv = 1;
        }
        else {
            sv = 2;
        }

        if (vecs[1][0]) {
            tv = 0;
        }
        else if (vecs[1][1]) {
            tv = 1;
        }
        else {
            tv = 2;
        }

        for (int32_t i = 0; i < 2; i++) {
            float ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
            float nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
            vecs[i][sv] = ns;
            vecs[i][tv] = nt;
        }

        for (int32_t i = 0; i < 2; i++) {
            for (int32_t j = 0; j < 3; j++) {
                mappingVecs[i][j] = vecs[i][j] / scale[i];
            }
        }

        mappingVecs[0][3] = -(shift[0] / scale[0]);
        mappingVecs[1][3] = -(shift[1] / scale[1]);
    }

} // namespace mapFile

X_NAMESPACE_END
