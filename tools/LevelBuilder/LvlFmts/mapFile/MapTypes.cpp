#include "stdafx.h"
#include "MapTypes.h"

#include <Util\BitUtil.h>

X_NAMESPACE_BEGIN(level)

namespace mapFile
{
    XMapPatch::XMapPatch(core::MemoryArenaBase* arena) :
        XMapPatch(arena, 0, 0)
    {
    }

    XMapPatch::XMapPatch(core::MemoryArenaBase* arena, int w, int h) :
        XMapPrimitive(PrimType::PATCH),
        verts_(arena),
        indexes_(arena),
        edges_(arena),
        edgeIndexes_(arena),
        width_(w),
        height_(h),
        maxWidth_(w),
        maxHeight_(h),
        horzSubdivisions_(0),
        vertSubdivisions_(0),
        isMesh_(false),
        expanded_(false)
    {
    }

    XMapPatch::~XMapPatch(void)
    {
    }

    void XMapPatch::GenerateEdgeIndexes(void)
    {
        int i, j;
        int i0, i1, i2, s, v0, v1, edgeNum;
        int *index, *vertexEdges, *edgeChain;
        SurfaceEdge e[3];

        vertexEdges = reinterpret_cast<int*>(Alloca16(verts_.size() * sizeof(int)));
        memset(vertexEdges, -1, verts_.size() * sizeof(int));
        edgeChain = reinterpret_cast<int*>(Alloca16(indexes_.size() * sizeof(int)));

        edgeIndexes_.resize(indexes_.size());

        edges_.clear();

        // the first edge is a dummy
        e[0].verts[0] = e[0].verts[1] = e[0].tris[0] = e[0].tris[1] = 0;
        edges_.push_back(e[0]);

        for (i = 0; i < safe_static_cast<int, size_t>(indexes_.size()); i += 3) {
            index = indexes_.ptr() + i;
            // vertex numbers
            i0 = index[0];
            i1 = index[1];
            i2 = index[2];
            // setup edges_ each with smallest vertex number first
            s = core::bitUtil::isSignBitSet(i1 - i0);
            e[0].verts[0] = index[s];
            e[0].verts[1] = index[s ^ 1];
            s = core::bitUtil::isSignBitSet(i2 - i1) + 1;
            e[1].verts[0] = index[s];
            e[1].verts[1] = index[s ^ 3];
            s = core::bitUtil::isSignBitSet(i2 - i0) << 1;
            e[2].verts[0] = index[s];
            e[2].verts[1] = index[s ^ 2];
            // get edges_
            for (j = 0; j < 3; j++) {
                v0 = e[j].verts[0];
                v1 = e[j].verts[1];
                for (edgeNum = vertexEdges[v0]; edgeNum >= 0; edgeNum = edgeChain[edgeNum]) {
                    if (edges_[edgeNum].verts[1] == v1) {
                        break;
                    }
                }
                // if the edge does not yet exist
                if (edgeNum < 0) {
                    e[j].tris[0] = e[j].tris[1] = -1;
                    //	edgeNum = edges_.push_back(e[j]);
                    edges_.push_back(e[j]);
                    edgeNum = (int)(edges_.size() - 1);

                    edgeChain[edgeNum] = vertexEdges[v0];
                    vertexEdges[v0] = edgeNum;
                }
                // update edge index and edge tri references
                if (index[j] == v0) {
                    //			assert(edges_[edgeNum].tris[0] == -1); // edge may not be shared by more than two triangles
                    edges_[edgeNum].tris[0] = i;
                    edgeIndexes_[i + j] = edgeNum;
                }
                else {
                    //			assert(edges_[edgeNum].tris[1] == -1); // edge may not be shared by more than two triangles
                    edges_[edgeNum].tris[1] = i;
                    edgeIndexes_[i + j] = -edgeNum;
                }
            }
        }
    }

    void XMapPatch::PutOnCurve(void)
    {
        X_ASSERT(expanded_, "needs to be exapanded")
        (expanded_);
        LvlVert prev, next;

        // put all the approximating points on the curve
        for (int32_t i = 0; i < width_; i++) {
            for (int32_t j = 1; j < height_; j += 2) {
                LerpVert(verts_[j * maxWidth_ + i], verts_[(j + 1) * maxWidth_ + i], prev);
                LerpVert(verts_[j * maxWidth_ + i], verts_[(j - 1) * maxWidth_ + i], next);
                LerpVert(prev, next, verts_[j * maxWidth_ + i]);
            }
        }

        for (int32_t j = 0; j < height_; j++) {
            for (int32_t i = 1; i < width_; i += 2) {
                LerpVert(verts_[j * maxWidth_ + i], verts_[j * maxWidth_ + i + 1], prev);
                LerpVert(verts_[j * maxWidth_ + i], verts_[j * maxWidth_ + i - 1], next);
                LerpVert(prev, next, verts_[j * maxWidth_ + i]);
            }
        }
    }

    void XMapPatch::ProjectPointOntoVector(const Vec3f& point, const Vec3f& vStart,
        const Vec3f& vEnd, Vec3f& vProj)
    {
        Vec3f pVec, vec;

        pVec = point - vStart;
        vec = vEnd - vStart;
        vec.normalize();
        // project onto the directional vector for this segment
        vProj = vStart + (pVec * vec) * vec;
    }

    /*

	Expects an expanded_ patch.
	*/
    void XMapPatch::RemoveLinearColumnsRows(void)
    {
        int32_t i, j, k;
        float len, maxLength;
        Vec3f proj, dir;

        X_ASSERT(expanded_, "needs to be exapanded")
        (expanded_);

        for (j = 1; j < width_ - 1; j++) {
            maxLength = 0;
            for (i = 0; i < height_; i++) {
                ProjectPointOntoVector(verts_[i * maxWidth_ + j].pos,
                    verts_[i * maxWidth_ + j - 1].pos, verts_[i * maxWidth_ + j + 1].pos, proj);
                dir = verts_[i * maxWidth_ + j].pos - proj;
                len = dir.lengthSquared();
                if (len > maxLength) {
                    maxLength = len;
                }
            }
            if (maxLength < math<float>::square(0.2f)) {
                width_--;
                for (i = 0; i < height_; i++) {
                    for (k = j; k < width_; k++) {
                        verts_[i * maxWidth_ + k] = verts_[i * maxWidth_ + k + 1];
                    }
                }
                j--;
            }
        }
        for (j = 1; j < height_ - 1; j++) {
            maxLength = 0;
            for (i = 0; i < width_; i++) {
                ProjectPointOntoVector(verts_[j * maxWidth_ + i].pos,
                    verts_[(j - 1) * maxWidth_ + i].pos, verts_[(j + 1) * maxWidth_ + i].pos, proj);
                dir = verts_[j * maxWidth_ + i].pos - proj;
                len = dir.lengthSquared();
                if (len > maxLength) {
                    maxLength = len;
                }
            }
            if (maxLength < math<float>::square(0.2f)) {
                height_--;
                for (i = 0; i < width_; i++) {
                    for (k = j; k < height_; k++) {
                        verts_[k * maxWidth_ + i] = verts_[(k + 1) * maxWidth_ + i];
                    }
                }
                j--;
            }
        }
    }

    void XMapPatch::ResizeExpanded(int32_t newHeight, int32_t newWidth)
    {
        int32_t i, j; // must be signed

        X_ASSERT(expanded_, "needs to be exapanded")
        (expanded_);
        if (newHeight <= maxHeight_ && newWidth <= maxWidth_) {
            return;
        }
        if (newHeight * newWidth > maxHeight_ * maxWidth_) {
            verts_.resize(newHeight * newWidth);
        }
        // space out verts_ for new height_ and width_
        for (j = safe_static_cast<int32_t>(maxHeight_) - 1; j >= 0; j--) {
            for (i = safe_static_cast<int32>(maxWidth_) - 1; i >= 0; i--) {
                verts_[j * newWidth + i] = verts_[j * maxWidth_ + i];
            }
        }
        maxHeight_ = newHeight;
        maxWidth_ = newWidth;
    }

    void XMapPatch::Collapse(void)
    {
        if (!expanded_) {
            X_FATAL("Patch", "patch not expanded_");
        }
        expanded_ = false;

        if (width_ != maxWidth_) {
            for (int32_t j = 0; j < height_; j++) {
                for (int32_t i = 0; i < width_; i++) {
                    verts_[j * width_ + i] = verts_[j * maxWidth_ + i];
                }
            }
        }
        verts_.resize(width_ * height_);
    }

    void XMapPatch::Expand(void)
    {
        if (expanded_) {
            X_FATAL("Patch", "patch alread expanded_");
        }

        expanded_ = true;

        verts_.resize(maxWidth_ * maxHeight_);

        if (width_ != maxWidth_) {
            for (int32_t j = safe_static_cast<int32_t>(height_) - 1; j >= 0; j--) {
                static_assert(core::compileTime::IsSigned<decltype(j)>::Value, "Must be signed");
                for (int32_t i = safe_static_cast<int32_t>(width_) - 1; i >= 0; i--) {
                    verts_[j * maxWidth_ + i] = verts_[j * width_ + i];
                }
            }
        }
    }

    void XMapPatch::LerpVert(const LvlVert& a, const LvlVert& b, LvlVert& out) const
    {
        out.pos = (0.5f * (a.pos + b.pos));
        out.normal = 0.5f * (a.normal + b.normal);
        out.uv = 0.5f * (a.uv + b.uv);
    }

    /*
	Handles all the complicated wrapping and degenerate cases
	Expects a Not expanded_ patch.
	*/

    void XMapPatch::GenerateNormals(void)
    {
        int32_t i, j, k, dist;
        Vec3f norm;
        Vec3f sum;
        int32_t count;
        Vec3f base;
        Vec3f delta;
        int32_t x, y;
        Vec3f around[8], temp;
        bool good[8];
        bool wrapWidth, wrapHeight;
        static int32_t neighbors[8][2] = {
            {0, 1}, {1, 1}, {1, 0}, {1, -1},
            {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}};

        X_ASSERT(!expanded_, "Patch must not be expanded before generating normals.")
        (expanded_);

        // if all points are coplanar, set all normals to that plane
        Vec3f extent[3];
        float offset;
        float length;

        extent[0] = verts_[width_ - 1].pos - verts_[0].pos;
        extent[1] = verts_[(height_ - 1) * width_ + width_ - 1].pos - verts_[0].pos;
        extent[2] = verts_[(height_ - 1) * width_].pos - verts_[0].pos;

        norm = extent[0].cross(extent[1]);
        if (norm.lengthSquared() == 0.0f) {
            norm = extent[0].cross(extent[2]);
            if (norm.lengthSquared() == 0.0f) {
                norm = extent[1].cross(extent[2]);
            }
        }

        // wrapped patched may not get a valid normal here
        length = norm.length();
        norm.normalize();
        if (length != 0.0f) {
            offset = verts_[0].pos * norm;
            for (i = 1; i < width_ * height_; i++) {
                float d = verts_[i].pos * norm;
                if (math<float>::abs(d - offset) > COPLANAR_EPSILON) {
                    break;
                }
            }

            if (i == width_ * height_) {
                // all are coplanar
                for (i = 0; i < width_ * height_; i++) {
                    verts_[i].normal = norm;
                }
                return;
            }
        }

        // check for wrapped edge cases, which should smooth across themselves
        wrapWidth = false;

        for (i = 0; i < height_; i++) {
            delta = verts_[i * width_].pos - verts_[i * width_ + width_ - 1].pos;
            if (delta.lengthSquared() > math<float>::square(1.0f)) {
                break;
            }
        }
        if (i == height_) {
            wrapWidth = true;
        }

        wrapHeight = false;
        for (i = 0; i < width_; i++) {
            delta = verts_[i].pos - verts_[(height_ - 1) * width_ + i].pos;
            if (delta.lengthSquared() > math<float>::square(1.0f)) {
                break;
            }
        }

        if (i == width_) {
            wrapHeight = true;
        }

        for (i = 0; i < width_; i++) {
            for (j = 0; j < height_; j++) {
                count = 0;
                base = verts_[j * width_ + i].pos;
                for (k = 0; k < 8; k++) {
                    around[k] = Vec3f::zero();
                    good[k] = false;

                    for (dist = 1; dist <= 3; dist++) {
                        x = i + neighbors[k][0] * dist;
                        y = j + neighbors[k][1] * dist;
                        if (wrapWidth) {
                            if (x < 0) {
                                x = width_ - 1 + x;
                            }
                            else if (x >= width_) {
                                x = 1 + x - width_;
                            }
                        }
                        if (wrapHeight) {
                            if (y < 0) {
                                y = height_ - 1 + y;
                            }
                            else if (y >= height_) {
                                y = 1 + y - height_;
                            }
                        }

                        if (x < 0 || x >= width_ || y < 0 || y >= height_) {
                            break; // edge of patch
                        }

                        temp = verts_[y * width_ + x].pos - base;

                        length = norm.length();
                        norm.normalize();

                        if (length == 0.0f) {
                            continue; // degenerate edge, get more dist
                        }
                        else {
                            good[k] = true;
                            around[k] = temp;
                            break; // good edge
                        }
                    }
                }

                sum = Vec3f::zero();
                for (k = 0; k < 8; k++) {
                    if (!good[k] || !good[(k + 1) & 7]) {
                        continue; // didn't get two points
                    }

                    norm = around[(k + 1) & 7].cross(around[k]);

                    length = norm.length();
                    norm.normalize();

                    if (length == 0.0f) {
                        continue;
                    }

                    sum += norm;
                    count++;
                }

                if (count == 0) {
                    count = 1;
                }

                verts_[j * width_ + i].normal = sum;
                verts_[j * width_ + i].normal.normalize();
            }
        }
    }

    void XMapPatch::GenerateIndexes(void)
    {
        indexes_.resize((width_ - 1) * (height_ - 1) * 2 * 3, false);

        int32_t index = 0;
        for (int32_t i = 0; i < width_ - 1; i++) {
            for (int32_t j = 0; j < height_ - 1; j++) {
                int32_t v1 = j * width_ + i;
                int32_t v2 = v1 + 1;
                int32_t v3 = v1 + width_ + 1;
                int32_t v4 = v1 + width_;
                indexes_[index++] = safe_static_cast<int32_t>(v1);
                indexes_[index++] = safe_static_cast<int32_t>(v3);
                indexes_[index++] = safe_static_cast<int32_t>(v2);
                indexes_[index++] = safe_static_cast<int32_t>(v1);
                indexes_[index++] = safe_static_cast<int32_t>(v4);
                indexes_[index++] = safe_static_cast<int32_t>(v3);
            }
        }

        GenerateEdgeIndexes();
    }

    void XMapPatch::Subdivide(float maxHorizontalError, float maxVerticalError,
        float maxLength, bool genNormals)
    {
        int32_t i, j, k, l;
        LvlVert prev, next, mid;
        Vec3f prevxyz, nextxyz, midxyz;
        Vec3f delta;
        float maxHorizontalErrorSqr, maxVerticalErrorSqr, maxLengthSqr;

        // generate normals for the control mesh
        if (genNormals) {
            GenerateNormals();
        }

        maxHorizontalErrorSqr = math<float>::square(maxHorizontalError);
        maxVerticalErrorSqr = math<float>::square(maxVerticalError);
        maxLengthSqr = math<float>::square(maxLength);

        Expand();

        // horizontal subdivisions
        for (j = 0; j + 2 < width_; j += 2) {
            // check subdivided midpoints against control points
            for (i = 0; i < height_; i++) {
                for (l = 0; l < 3; l++) {
                    prevxyz[l] = verts_[i * maxWidth_ + j + 1].pos[l] - verts_[i * maxWidth_ + j].pos[l];

                    nextxyz[l] = verts_[i * maxWidth_ + j + 2].pos[l] - verts_[i * maxWidth_ + j + 1].pos[l];

                    midxyz[l] = (verts_[i * maxWidth_ + j].pos[l] + verts_[i * maxWidth_ + j + 1].pos[l] * 2.0f + verts_[i * maxWidth_ + j + 2].pos[l]) * 0.25f;
                }

                if (maxLength > 0.0f) {
                    // if the span length is too long, force a subdivision
                    if (prevxyz.lengthSquared() > maxLengthSqr || nextxyz.lengthSquared() > maxLengthSqr) {
                        break;
                    }
                }

                // see if this midpoint is off far enough to subdivide
                delta = verts_[i * maxWidth_ + j + 1].pos - midxyz;

                if (delta.lengthSquared() > maxHorizontalErrorSqr) {
                    break;
                }
            }

            if (i == height_) {
                continue; // didn't need subdivision
            }

            if (width_ + 2 >= maxWidth_) {
                ResizeExpanded(maxHeight_, maxWidth_ + 4);
            }

            // insert two columns and replace the peak
            width_ += 2;

            for (i = 0; i < height_; i++) {
                LerpVert(verts_[i * maxWidth_ + j], verts_[i * maxWidth_ + j + 1], prev);
                LerpVert(verts_[i * maxWidth_ + j + 1], verts_[i * maxWidth_ + j + 2], next);
                LerpVert(prev, next, mid);

                for (k = width_ - 1; k > j + 3; k--) {
                    verts_[i * maxWidth_ + k] = verts_[i * maxWidth_ + k - 2];
                }

                verts_[i * maxWidth_ + j + 1] = prev;
                verts_[i * maxWidth_ + j + 2] = mid;
                verts_[i * maxWidth_ + j + 3] = next;
            }

            // back up and recheck this set again, it may need more subdivision
            j -= 2;
        }

        // vertical subdivisions
        for (j = 0; j + 2 < height_; j += 2) {
            // check subdivided midpoints against control points
            for (i = 0; i < width_; i++) {
                for (l = 0; l < 3; l++) {
                    prevxyz[l] = verts_[(j + 1) * maxWidth_ + i].pos[l] - verts_[j * maxWidth_ + i].pos[l];

                    nextxyz[l] = verts_[(j + 2) * maxWidth_ + i].pos[l] - verts_[(j + 1) * maxWidth_ + i].pos[l];

                    midxyz[l] = (verts_[j * maxWidth_ + i].pos[l] + verts_[(j + 1) * maxWidth_ + i].pos[l] * 2.0f + verts_[(j + 2) * maxWidth_ + i].pos[l]) * 0.25f;
                }

                if (maxLength > 0.0f) {
                    // if the span length is too long, force a subdivision
                    if (prevxyz.lengthSquared() > maxLengthSqr || nextxyz.lengthSquared() > maxLengthSqr) {
                        break;
                    }
                }

                // see if this midpoint is off far enough to subdivide
                delta = verts_[(j + 1) * maxWidth_ + i].pos - midxyz;
                if (delta.lengthSquared() > maxVerticalErrorSqr) {
                    break;
                }
            }

            if (i == width_) {
                continue; // didn't need subdivision
            }

            if (height_ + 2 >= maxHeight_) {
                ResizeExpanded(maxHeight_ + 4, maxWidth_);
            }

            // insert two columns and replace the peak
            height_ += 2;

            for (i = 0; i < width_; i++) {
                LerpVert(verts_[j * maxWidth_ + i], verts_[(j + 1) * maxWidth_ + i], prev);
                LerpVert(verts_[(j + 1) * maxWidth_ + i], verts_[(j + 2) * maxWidth_ + i], next);
                LerpVert(prev, next, mid);

                for (k = height_ - 1; k > j + 3; k--) {
                    verts_[k * maxWidth_ + i] = verts_[(k - 2) * maxWidth_ + i];
                }

                verts_[(j + 1) * maxWidth_ + i] = prev;
                verts_[(j + 2) * maxWidth_ + i] = mid;
                verts_[(j + 3) * maxWidth_ + i] = next;
            }

            // back up and recheck this set again, it may need more subdivision
            j -= 2;
        }

        PutOnCurve();

        RemoveLinearColumnsRows();

        Collapse();

        // normalize all the lerped normals
        if (genNormals) {
            for (i = 0; i < width_ * height_; i++) {
                verts_[i].normal.normalize();
            }
        }

        GenerateIndexes();
    }

    void XMapPatch::SampleSinglePatchPoint(const LvlVert ctrl[3][3], float u, float v, LvlVert* out) const
    {
        float vCtrl[3][8];
        int vPoint;
        int axis;

        // find the control points for the v coordinate
        for (vPoint = 0; vPoint < 3; vPoint++) {
            for (axis = 0; axis < 8; axis++) {
                float a, b, c;
                float qA, qB, qC;
                if (axis < 3) {
                    a = ctrl[0][vPoint].pos[axis];
                    b = ctrl[1][vPoint].pos[axis];
                    c = ctrl[2][vPoint].pos[axis];
                }
                else if (axis < 6) {
                    a = ctrl[0][vPoint].normal[axis - 3];
                    b = ctrl[1][vPoint].normal[axis - 3];
                    c = ctrl[2][vPoint].normal[axis - 3];
                }
                else {
                    a = ctrl[0][vPoint].uv[axis - 6];
                    b = ctrl[1][vPoint].uv[axis - 6];
                    c = ctrl[2][vPoint].uv[axis - 6];
                }
                qA = a - 2.0f * b + c;
                qB = 2.0f * b - 2.0f * a;
                qC = a;
                vCtrl[vPoint][axis] = qA * u * u + qB * u + qC;
            }
        }

        // interpolate the v value
        for (axis = 0; axis < 8; axis++) {
            float a, b, c;
            float qA, qB, qC;

            a = vCtrl[0][axis];
            b = vCtrl[1][axis];
            c = vCtrl[2][axis];
            qA = a - 2.0f * b + c;
            qB = 2.0f * b - 2.0f * a;
            qC = a;

            if (axis < 3) {
                out->pos[axis] = qA * v * v + qB * v + qC;
            }
            else if (axis < 6) {
                out->normal[axis - 3] = qA * v * v + qB * v + qC;
            }
            else {
                out->uv[axis - 6] = qA * v * v + qB * v + qC;
            }
        }
    }

    void XMapPatch::SampleSinglePatch(const LvlVert ctrl[3][3], int32_t baseCol, int32_t baseRow,
        int32_t width, int32_t horzSub, int32_t vertSub, LvlVert* outVerts) const
    {
        horzSub++;
        vertSub++;

        for (int32_t i = 0; i < horzSub; i++) {
            for (int32_t j = 0; j < vertSub; j++) {
                float u = static_cast<float>(i) / (horzSub - 1);
                float v = static_cast<float>(j) / (vertSub - 1);
                SampleSinglePatchPoint(ctrl, u, v, &outVerts[((baseRow + j) * width) + i + baseCol]);
            }
        }
    }

    void XMapPatch::SubdivideExplicit(int32_t horzSubdivisions, int32_t vertSubdivisions,
        bool genNormals, bool removeLinear)
    {
        int32_t i, j, k, l;
        LvlVert sample[3][3];
        int32_t outWidth = ((width_ - 1) / 2 * horzSubdivisions) + 1;
        int32_t outHeight = ((height_ - 1) / 2 * vertSubdivisions) + 1;
        LvlVert* dv = X_NEW_ARRAY(LvlVert, (outWidth * outHeight), g_arena, "PatchSubDivideVerts");

        // generate normals for the control mesh
        if (genNormals) {
            GenerateNormals();
        }

        int32_t baseCol = 0;
        for (i = 0; i + 2 < width_; i += 2) {
            int32_t baseRow = 0;
            for (j = 0; j + 2 < height_; j += 2) {
                for (k = 0; k < 3; k++) {
                    for (l = 0; l < 3; l++) {
                        sample[k][l] = verts_[((j + l) * width_) + i + k];
                    }
                }

                SampleSinglePatch(sample, baseCol, baseRow, outWidth,
                    horzSubdivisions, vertSubdivisions, dv);

                baseRow += vertSubdivisions;
            }

            baseCol += horzSubdivisions;
        }

        verts_.resize(outWidth * outHeight);
        for (i = 0; i < outWidth * outHeight; i++) {
            verts_[i] = dv[i];
        }

        X_DELETE_ARRAY(dv, g_arena);

        width_ = maxWidth_ = outWidth;
        height_ = maxHeight_ = outHeight;
        expanded_ = false;

        if (removeLinear) {
            Expand();
            RemoveLinearColumnsRows();
            Collapse();
        }

        // normalize all the lerped normals
        if (genNormals) {
            for (i = 0; i < width_ * height_; i++) {
                verts_[i].normal.normalize();
            }
        }

        GenerateIndexes();
    }

    void XMapPatch::CreateNormalsAndIndexes(void)
    {
        GenerateNormals();
        GenerateIndexes();
    }

} // namespace mapFile

X_NAMESPACE_END
