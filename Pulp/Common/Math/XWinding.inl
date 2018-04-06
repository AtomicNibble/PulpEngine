

template<class Allocator>
XWindingT<Allocator>::XWindingT(void) :
    pPoints_(nullptr),
    numPoints_(0),
    allocedSize_(0)
{
}

// allocate for n points
template<class Allocator>
XWindingT<Allocator>::XWindingT(const size_t n) :
    pPoints_(nullptr),
    numPoints_(0),
    allocedSize_(0)
{
    EnsureAlloced(n);
}

// winding from points
template<class Allocator>
XWindingT<Allocator>::XWindingT(const Vec3f* pVerts, const size_t numVerts) :
    pPoints_(nullptr),
    numPoints_(safe_static_cast<int32_t, size_t>(numVerts)),
    allocedSize_(0)
{
    EnsureAlloced(numVerts);
    for (size_t i = 0; i < numVerts; i++) {
        pPoints_[i] = Vec5f(pVerts[i]);
    }
}

// winding from points
template<class Allocator>
XWindingT<Allocator>::XWindingT(const Vec5f* pVerts, const size_t numVerts) :
    pPoints_(nullptr),
    numPoints_(safe_static_cast<int32_t, size_t>(numVerts)),
    allocedSize_(0)
{
    EnsureAlloced(numVerts);
    std::copy_n(pVerts, numVerts, pPoints_);
}

// base winding for plane
template<class Allocator>
XWindingT<Allocator>::XWindingT(const Vec3f& normal, const float dist) :
    pPoints_(nullptr),
    numPoints_(0),
    allocedSize_(0)
{
    baseForPlane(normal, dist);
}

// base winding for plane
template<class Allocator>
XWindingT<Allocator>::XWindingT(const Planef& plane) :
    pPoints_(nullptr),
    numPoints_(0),
    allocedSize_(0)
{
    baseForPlane(plane);
}

template<class Allocator>
XWindingT<Allocator>::XWindingT(const MyType& winding) :
    pPoints_(nullptr),
    numPoints_(0),
    allocedSize_(0)
{
    EnsureAlloced(winding.getNumPoints());
    numPoints_ = winding.numPoints_;
    std::copy_n(winding.pPoints_, winding.numPoints_, pPoints_);
}

template<class Allocator>
XWindingT<Allocator>::XWindingT(MyType&& oth) :
    pPoints_(oth.pPoints_),
    numPoints_(oth.numPoints_),
    allocedSize_(oth.allocedSize_)
{
    oth.numPoints_ = 0;
    oth.allocedSize_ = 0;
    oth.pPoints_ = nullptr;
}

template<class Allocator>
XWindingT<Allocator>::~XWindingT(void)
{
    free();
}

template<class Allocator>
XWindingT<Allocator>& XWindingT<Allocator>::operator=(const XWindingT& rhs)
{
    if (this != &rhs) {
        EnsureAlloced(rhs.getNumPoints());
        numPoints_ = rhs.numPoints_;
        std::copy_n(rhs.pPoints_, rhs.numPoints_, pPoints_);
    }
    return *this;
}

template<class Allocator>
XWindingT<Allocator>& XWindingT<Allocator>::operator=(XWindingT&& rhs)
{
    numPoints_ = rhs.numPoints_;
    allocedSize_ = rhs.allocedSize_;
    pPoints_ = rhs.pPoints_;

    rhs.numPoints_ = 0;
    rhs.allocedSize_ = 0;
    rhs.pPoints_ = nullptr;
    return *this;
}

// ----------------------------------------------------------------------

template<class Allocator>
bool XWindingT<Allocator>::isTiny(void) const
{
    int32_t edges = 0;

    for (int32_t i = 0; i < numPoints_; i++) {
        Vec3f delta = pPoints_[(i + 1) % numPoints_].asVec3() - pPoints_[i].asVec3();
        float len = delta.length();
        if (len > EDGE_LENGTH) {
            if (++edges == 3) {
                return false;
            }
        }
    }
    return true;
}

template<class Allocator>
bool XWindingT<Allocator>::isHuge(void) const
{
    for (int32_t i = 0; i < numPoints_; i++) {
        for (int32_t j = 0; j < 3; j++) {
            if (pPoints_[i][j] <= MIN_WORLD_COORD || pPoints_[i][j] >= MAX_WORLD_COORD) {
                return true;
            }
        }
    }
    return false;
}

template<class Allocator>
void XWindingT<Allocator>::clear(void)
{
    numPoints_ = 0;
}

template<class Allocator>
void XWindingT<Allocator>::free(void)
{
    numPoints_ = 0;
    allocedSize_ = 0;
    allocator_.free(pPoints_);
    pPoints_ = nullptr;
}

template<class Allocator>
void XWindingT<Allocator>::print(void) const
{
    // print the bounds.
    AABB bounds;
    GetAABB(bounds);

    const Vec3f& min = bounds.min;
    const Vec3f& max = bounds.max;
    X_LOG0("Winding", "Printing values:");
    X_LOG_BULLET;
    X_LOG0("Winding", "min: (^8%g,%g,%g^7)", min[0], min[1], min[2]);
    X_LOG0("Winding", "max: (^8%g,%g,%g^7)", max[0], max[1], max[2]);
    X_LOG0("Winding", "NumPoints: ^8%i", numPoints_);
    for (int32_t i = 0; i < numPoints_; i++) {
        const Vec5f& pl = pPoints_[i];
        X_LOG0("Winding", "P(^8%i^7): (^8%g,%g,%g^7) (^8%g,%g^7)", i,
            pl[0], pl[1], pl[2], pl[3], pl[4]);
    }
}

// ----------------------------------------------------------------------

template<class Allocator>
void XWindingT<Allocator>::baseForPlane(const Vec3f& normal, const float dist)
{
    Vec3f org, vright, vup;

    org = normal * dist;

    NormalVectors(normal, vup, vright);

    vup *= static_cast<float>(MAX_WORLD_SIZE);
    vright *= static_cast<float>(MAX_WORLD_SIZE);

    EnsureAlloced(4);
    numPoints_ = 4;
    pPoints_[0] = org - vright + vup;
    pPoints_[1] = org + vright + vup;
    pPoints_[2] = org + vright - vup;
    pPoints_[3] = org - vright - vup;
}

template<class Allocator>
void XWindingT<Allocator>::baseForPlane(const Planef& plane)
{
    baseForPlane(plane.getNormal(), plane.getDistance());
}

// ----------------------------------------------------------------------

template<class Allocator>
float XWindingT<Allocator>::getArea(void) const
{
    Vec3f d1, d2, cross;

    float total = 0.0f;
    for (int32_t i = 2; i < numPoints_; i++) {
        d1 = pPoints_[i - 1].asVec3() - pPoints_[0].asVec3();
        d2 = pPoints_[i].asVec3() - pPoints_[0].asVec3();
        cross = d1.cross(d2);
        total += cross.length();
    }
    return total * 0.5f;
}

template<class Allocator>
Vec3f XWindingT<Allocator>::getCenter(void) const
{
    Vec3f center = Vec3f::zero();
    for (int32_t i = 0; i < numPoints_; i++) {
        center += pPoints_[i].asVec3();
    }
    center *= (1.0f / numPoints_);
    return center;
}

template<class Allocator>
float XWindingT<Allocator>::getRadius(const Vec3f& center) const
{
    float radius, r;
    Vec3f dir;

    radius = 0.0f;
    for (int32_t i = 0; i < numPoints_; i++) {
        dir = pPoints_[i].asVec3() - center;
        r = dir * dir;
        if (r > radius) {
            radius = r;
        }
    }
    return math<float>::sqrt(radius);
}

template<class Allocator>
void XWindingT<Allocator>::getPlane(Vec3f& normal, float& dist) const
{
    Vec3f v1, v2, center;

    if (numPoints_ < 3) {
        normal = Vec3f::zero();
        dist = 0.0f;
        return;
    }

    center = getCenter();
    v1 = pPoints_[0].asVec3() - center;
    v2 = pPoints_[1].asVec3() - center;
    normal = v2.cross(v1);
    normal.normalize();
    dist = pPoints_[0].asVec3() * normal;
}

template<class Allocator>
void XWindingT<Allocator>::getPlane(Planef& plane) const
{
    Vec3f v1, v2;
    Vec3f center;

    if (numPoints_ < 3) {
        //	plane.set(Vec3f::zero(), 0);
        core::zero_object(plane);
        return;
    }

    center = getCenter();
    v1 = pPoints_[0].asVec3() - center;
    v2 = pPoints_[1].asVec3() - center;
    plane.setNormal(v2.cross(v1).normalized());
    plane.setDistance(plane.getNormal() * pPoints_[0].asVec3());
}

template<class Allocator>
void XWindingT<Allocator>::GetAABB(AABB& box) const
{
    if (!numPoints_) {
        box.clear();
        return;
    }

    box.min = box.max = pPoints_[0].asVec3();

    for (int32_t i = 1; i < numPoints_; i++) {
        if (pPoints_[i].x < box.min.x) {
            box.min.x = pPoints_[i].x;
        }
        else if (pPoints_[i].x > box.max.x) {
            box.max.x = pPoints_[i].x;
        }

        if (pPoints_[i].y < box.min.y) {
            box.min.y = pPoints_[i].y;
        }
        else if (pPoints_[i].y > box.max.y) {
            box.max.y = pPoints_[i].y;
        }

        if (pPoints_[i].z < box.min.z) {
            box.min.z = pPoints_[i].z;
        }
        else if (pPoints_[i].z > box.max.z) {
            box.max.z = pPoints_[i].z;
        }
    }
}

template<class Allocator>
float XWindingT<Allocator>::planeDistance(const Planef& plane) const
{
    using namespace core::FloatUtil;

    float min = INFINITY;
    float max = -min;
    for (int32_t i = 0; i < numPoints_; i++) {
        float d = plane.distance(pPoints_[i].asVec3());
        if (d < min) {
            min = d;

            if (isSignBitSet(min) && isSignBitNotSet(max)) {
                return 0.0f;
            }
        }
        if (d > max) {
            max = d;

            if (isSignBitSet(min) && isSignBitNotSet(max)) {
                return 0.0f;
            }
        }
    }
    if (isSignBitNotSet(min)) {
        return min;
    }
    if (isSignBitSet(max)) {
        return max;
    }
    return 0.0f;
}

template<class Allocator>
PlaneSide::Enum XWindingT<Allocator>::planeSide(const Planef& plane, const float epsilon) const
{
    bool front = false;
    bool back = false;
    for (int32_t i = 0; i < numPoints_; i++) {
        float d = plane.distance(pPoints_[i].asVec3());
        if (d < -epsilon) {
            if (front) {
                return PlaneSide::CROSS;
            }
            back = true;
            continue;
        }
        else if (d > epsilon) {
            if (back) {
                return PlaneSide::CROSS;
            }
            front = true;
            continue;
        }
    }

    if (back) {
        return PlaneSide::BACK;
    }
    if (front) {
        return PlaneSide::FRONT;
    }
    return PlaneSide::ON;
}

// cuts off the part at the back side of the plane, returns true if some part was at the front
// if there is nothing at the front the number of points is set to zero
template<class Allocator>
bool XWindingT<Allocator>::clipInPlace(const Planef& plane, const float epsilon, const bool keepOn)
{
    float* dists = static_cast<float*>(_alloca((numPoints_ + 4) * sizeof(float)));
    uint8_t* sides = static_cast<uint8_t*>(_alloca((numPoints_ + 4) * sizeof(uint8_t)));

    int32_t counts[3] = {};
    float dot;

    // determine sides for each point
    int32_t i;
    for (i = 0; i < numPoints_; i++) {
        dists[i] = dot = plane.distance(pPoints_[i].asVec3());
        if (dot > epsilon) {
            sides[i] = PlaneSide::FRONT;
        }
        else if (dot < -epsilon) {
            sides[i] = PlaneSide::BACK;
        }
        else {
            sides[i] = PlaneSide::ON;
        }

        counts[sides[i]]++;
    }
    sides[i] = sides[0];
    dists[i] = dists[0];

    // if the winding is on the plane and we should keep it
    if (keepOn && !counts[PlaneSide::FRONT] && !counts[PlaneSide::BACK]) {
        return true;
    }
    // if nothing at the front of the clipping plane
    if (!counts[PlaneSide::FRONT]) {
        numPoints_ = 0;
        return false;
    }
    // if nothing at the back of the clipping plane
    if (!counts[PlaneSide::BACK]) {
        return true;
    }

    const int32_t maxpts = numPoints_ + 4; // cant use counts[0]+2 because of fp grouping errors

    Vec5f* pNewPoints = reinterpret_cast<Vec5f*>(alloca16(maxpts * sizeof(Vec5f)));
    int32_t newNumPoints = 0;
    Vec5f mid;

    for (i = 0; i < numPoints_; i++) {
        auto& p1 = pPoints_[i];

        if (newNumPoints + 1 > maxpts) {
            return true; // can't split -- fall back to original
        }

        if (sides[i] == PlaneSide::ON) {
            pNewPoints[newNumPoints] = p1;
            newNumPoints++;
            continue;
        }

        if (sides[i] == PlaneSide::FRONT) {
            pNewPoints[newNumPoints] = p1;
            newNumPoints++;
        }

        if (sides[i + 1] == PlaneSide::ON || sides[i + 1] == sides[i]) {
            continue;
        }

        if (newNumPoints + 1 > maxpts) {
            return true; // can't split -- fall back to original
        }

        // generate a split point
        auto& p2 = pPoints_[(i + 1) % numPoints_];

        dot = dists[i] / (dists[i] - dists[i + 1]);
        for (int32_t j = 0; j < 3; j++) {
            // avoid round off error when possible
            if (plane.getNormal()[j] == 1.0f) {
                mid[j] = plane.getDistance();
            }
            else if (plane.getNormal()[j] == -1.0f) {
                mid[j] = -plane.getDistance();
            }
            else {
                mid[j] = p1[j] + dot * (p2[j] - p1[j]);
            }
        }

        mid.s = p1.s + dot * (p2.s - p1.s);
        mid.t = p1.t + dot * (p2.t - p1.t);

        pNewPoints[newNumPoints] = mid;
        newNumPoints++;
    }

    EnsureAlloced(newNumPoints, false);
    numPoints_ = newNumPoints;
    memcpy(pPoints_, pNewPoints, newNumPoints * sizeof(Vec5f));
    return true;
}

template<class Allocator>
bool XWindingT<Allocator>::clip(const Planef& plane, const float epsilon, const bool keepOn)
{
    float dot;
    int32_t i;

    X_ASSERT_NOT_NULL(this);

    float dists[MAX_POINTS_ON_WINDING + 4];
    int32_t sides[MAX_POINTS_ON_WINDING + 4];
    int32_t counts[3] = {};

    // determine sides for each point
    for (i = 0; i < numPoints_; i++) {
        dists[i] = dot = plane.distance(pPoints_[i].asVec3());
        if (dot > epsilon) {
            sides[i] = PlaneSide::FRONT;
        }
        else if (dot < -epsilon) {
            sides[i] = PlaneSide::BACK;
        }
        else {
            sides[i] = PlaneSide::ON;
        }
        counts[sides[i]]++;
    }
    sides[i] = sides[0];
    dists[i] = dists[0];

    // if the winding is on the plane and we should keep it
    if (keepOn && !counts[PlaneSide::FRONT] && !counts[PlaneSide::BACK]) {
        return true;
    }
    // if nothing at the front of the clipping plane
    if (!counts[PlaneSide::FRONT]) {
        clear();
        return false;
    }
    // if nothing at the back of the clipping plane
    if (!counts[PlaneSide::BACK]) {
        return true;
    }

    const int32_t maxpts = numPoints_ + 4; // cant use counts[0]+2 because of fp grouping errors
    int32_t newNumPoints = 0;
    Vec5f* pNewPoints = reinterpret_cast<Vec5f*>(alloca16(maxpts * sizeof(Vec5f)));
    Vec5f mid;

    for (i = 0; i < numPoints_; i++) {
        auto& p1 = pPoints_[i];

        if (newNumPoints + 1 > maxpts) {
            return true; // can't split -- fall back to original
        }

        if (sides[i] == PlaneSide::ON) {
            pNewPoints[newNumPoints] = p1;
            newNumPoints++;
            continue;
        }

        if (sides[i] == PlaneSide::FRONT) {
            pNewPoints[newNumPoints] = p1;
            newNumPoints++;
        }

        if (sides[i + 1] == PlaneSide::ON || sides[i + 1] == sides[i]) {
            continue;
        }

        if (newNumPoints + 1 > maxpts) {
            return true; // can't split -- fall back to original
        }

        // generate a split point
        auto& p2 = pPoints_[(i + 1) % numPoints_];

        dot = dists[i] / (dists[i] - dists[i + 1]);
        for (int32_t j = 0; j < 3; j++) {
            // avoid round off error when possible
            if (plane.getNormal()[j] == 1.0f) {
                mid[j] = plane.getDistance();
            }
            else if (plane.getNormal()[j] == -1.0f) {
                mid[j] = -plane.getDistance();
            }
            else {
                mid[j] = p1[j] + dot * (p2[j] - p1[j]);
            }
        }

        mid.s = p1.s + dot * (p2.s - p1.s);
        mid.t = p1.t + dot * (p2.t - p1.t);

        pNewPoints[newNumPoints] = mid;
        newNumPoints++;
    }

    EnsureAlloced(newNumPoints, false);
    numPoints_ = newNumPoints;
    memcpy(pPoints_, pNewPoints, newNumPoints * sizeof(Vec5f));
    return true;
}

template<class Allocator>
XWindingT<Allocator>* XWindingT<Allocator>::Copy(core::MemoryArenaBase* arena) const
{
    return X_NEW(MyType, arena, "WindingCopy")(*this);
}

template<class Allocator>
XWindingT<Allocator>* XWindingT<Allocator>::Move(core::MemoryArenaBase* arena)
{
    return X_NEW(MyType, arena, "WindingCopy")(std::move(*this));
}

template<class Allocator>
XWindingT<Allocator>* XWindingT<Allocator>::ReverseWinding(core::MemoryArenaBase* arena) const
{
    MyType* pCopy = X_NEW(MyType, arena, "ReverseWinding");
    pCopy->EnsureAlloced(numPoints_);

    for (int32_t i = 0; i < numPoints_; i++) {
        pCopy->pPoints_[i] = pPoints_[numPoints_ - 1 - i];
    }
    pCopy->numPoints_ = numPoints_;
    return pCopy;
}

template<class Allocator>
PlaneSide::Enum XWindingT<Allocator>::Split(const Planef& plane, const float epsilon,
    XWindingT** pFront, XWindingT** pBack, core::MemoryArenaBase* arena) const
{
    *pFront = *pBack = nullptr;

    float dists[MAX_POINTS_ON_WINDING + 4];
    int32_t sides[MAX_POINTS_ON_WINDING + 4];
    int32_t counts[3] = {};

    // determine sides for each point
    int32_t i;
    for (i = 0; i < numPoints_; i++) {
        float dot = plane.distance(pPoints_[i].asVec3());
        dists[i] = dot;

        if (dot > epsilon) {
            sides[i] = PlaneSide::FRONT;
        }
        else if (dot < -epsilon) {
            sides[i] = PlaneSide::BACK;
        }
        else {
            sides[i] = PlaneSide::ON;
        }
        counts[sides[i]]++;
    }

    sides[i] = sides[0];
    dists[i] = dists[0];

    // if coplanar, put on the front side if the normals match
    if (!counts[PlaneSide::FRONT] && !counts[PlaneSide::BACK]) {
        Planef windingPlane;

        getPlane(windingPlane);
        if (windingPlane.getNormal() * plane.getNormal() > 0.0f) {
            *pFront = Copy(arena);
            return PlaneSide::FRONT;
        }
        else {
            *pBack = Copy(arena);
            return PlaneSide::BACK;
        }
    }
    // if nothing at the front of the clipping plane
    if (!counts[PlaneSide::FRONT]) {
        *pBack = Copy(arena);
        return PlaneSide::BACK;
    }
    // if nothing at the back of the clipping plane
    if (!counts[PlaneSide::BACK]) {
        *pFront = Copy(arena);
        return PlaneSide::FRONT;
    }

    const int32_t maxpts = numPoints_ + 4; // cant use counts[0]+2 because of fp grouping errors

    MyType *f, *b;
    *pFront = f = X_NEW(MyType, arena, "FrontWinding")(maxpts);
    *pBack = b = X_NEW(MyType, arena, "BackWinding")(maxpts);
    Vec5f mid;

    for (i = 0; i < numPoints_; i++) {
        const auto& p1 = pPoints_[i];

        if (sides[i] == PlaneSide::ON) {
            f->pPoints_[f->numPoints_] = p1;
            f->numPoints_++;
            b->pPoints_[b->numPoints_] = p1;
            b->numPoints_++;
            continue;
        }

        if (sides[i] == PlaneSide::FRONT) {
            f->pPoints_[f->numPoints_] = p1;
            f->numPoints_++;
        }

        if (sides[i] == PlaneSide::BACK) {
            b->pPoints_[b->numPoints_] = p1;
            b->numPoints_++;
        }

        if (sides[i + 1] == PlaneSide::ON || sides[i + 1] == sides[i]) {
            continue;
        }

        // generate a split point
        const auto& p2 = pPoints_[(i + 1) % numPoints_];

        // always calculate the split going from the same side
        // or minor epsilon issues can happen
        if (sides[i] == PlaneSide::FRONT) {
            float dot = dists[i] / (dists[i] - dists[i + 1]);
            for (int32_t j = 0; j < 3; j++) {
                // avoid round off error when possible
                if (plane.getNormal()[j] == 1.0f) {
                    mid[j] = plane.getDistance();
                }
                else if (plane.getNormal()[j] == -1.0f) {
                    mid[j] = -plane.getDistance();
                }
                else {
                    mid[j] = p1[j] + dot * (p2[j] - p1[j]);
                }
            }

            mid.s = p1.s + dot * (p2.s - p1.s);
            mid.t = p1.t + dot * (p2.t - p1.t);
        }
        else {
            float dot = dists[i + 1] / (dists[i + 1] - dists[i]);
            for (int32_t j = 0; j < 3; j++) {
                // avoid round off error when possible
                if (plane.getNormal()[j] == 1.0f) {
                    mid[j] = plane.getDistance();
                }
                else if (plane.getNormal()[j] == -1.0f) {
                    mid[j] = -plane.getDistance();
                }
                else {
                    mid[j] = p2[j] + dot * (p1[j] - p2[j]);
                }
            }

            mid.s = p2.s + dot * (p1.s - p2.s);
            mid.t = p2.t + dot * (p1.t - p2.t);
        }

        f->pPoints_[f->numPoints_] = mid;
        f->numPoints_++;
        b->pPoints_[b->numPoints_] = mid;
        b->numPoints_++;
    }

    if (f->numPoints_ > maxpts || b->numPoints_ > maxpts) {
        X_WARNING("XWinding", "points exceeded estimate");
    }

    return PlaneSide::CROSS;
}

template<class Allocator>
PlaneSide::Enum XWindingT<Allocator>::SplitMove(const Planef& plane, const float epsilon,
    XWindingT** pFront, XWindingT** pBack, core::MemoryArenaBase* arena)
{
    *pFront = *pBack = nullptr;

    float dists[MAX_POINTS_ON_WINDING + 4];
    int32_t sides[MAX_POINTS_ON_WINDING + 4];
    int32_t counts[3] = {};

    // determine sides for each point
    int32_t i;
    for (i = 0; i < numPoints_; i++) {
        float dot = plane.distance(pPoints_[i].asVec3());
        dists[i] = dot;

        if (dot > epsilon) {
            sides[i] = PlaneSide::FRONT;
        }
        else if (dot < -epsilon) {
            sides[i] = PlaneSide::BACK;
        }
        else {
            sides[i] = PlaneSide::ON;
        }
        counts[sides[i]]++;
    }

    sides[i] = sides[0];
    dists[i] = dists[0];

    // if coplanar, put on the front side if the normals match
    if (!counts[PlaneSide::FRONT] && !counts[PlaneSide::BACK]) {
        Planef windingPlane;

        getPlane(windingPlane);
        if (windingPlane.getNormal() * plane.getNormal() > 0.0f) {
            *pFront = Move(arena);
            return PlaneSide::FRONT;
        }
        else {
            *pBack = Move(arena);
            return PlaneSide::BACK;
        }
    }
    // if nothing at the front of the clipping plane
    if (!counts[PlaneSide::FRONT]) {
        *pBack = Move(arena);
        return PlaneSide::BACK;
    }
    // if nothing at the back of the clipping plane
    if (!counts[PlaneSide::BACK]) {
        *pFront = Move(arena);
        return PlaneSide::FRONT;
    }

    const int32_t maxpts = numPoints_ + 4; // cant use counts[0]+2 because of fp grouping errors

    MyType *f, *b;
    *pFront = f = X_NEW(MyType, arena, "FrontWinding")(maxpts);
    *pBack = b = X_NEW(MyType, arena, "BackWinding")(maxpts);
    Vec5f mid;

    for (i = 0; i < numPoints_; i++) {
        const auto& p1 = pPoints_[i];

        if (sides[i] == PlaneSide::ON) {
            f->pPoints_[f->numPoints_] = p1;
            f->numPoints_++;
            b->pPoints_[b->numPoints_] = p1;
            b->numPoints_++;
            continue;
        }

        if (sides[i] == PlaneSide::FRONT) {
            f->pPoints_[f->numPoints_] = p1;
            f->numPoints_++;
        }

        if (sides[i] == PlaneSide::BACK) {
            b->pPoints_[b->numPoints_] = p1;
            b->numPoints_++;
        }

        if (sides[i + 1] == PlaneSide::ON || sides[i + 1] == sides[i]) {
            continue;
        }

        // generate a split point
        const auto& p2 = pPoints_[(i + 1) % numPoints_];

        // always calculate the split going from the same side
        // or minor epsilon issues can happen
        if (sides[i] == PlaneSide::FRONT) {
            float dot = dists[i] / (dists[i] - dists[i + 1]);
            for (int32_t j = 0; j < 3; j++) {
                // avoid round off error when possible
                if (plane.getNormal()[j] == 1.0f) {
                    mid[j] = plane.getDistance();
                }
                else if (plane.getNormal()[j] == -1.0f) {
                    mid[j] = -plane.getDistance();
                }
                else {
                    mid[j] = p1[j] + dot * (p2[j] - p1[j]);
                }
            }

            mid.s = p1.s + dot * (p2.s - p1.s);
            mid.t = p1.t + dot * (p2.t - p1.t);
        }
        else {
            float dot = dists[i + 1] / (dists[i + 1] - dists[i]);
            for (int32_t j = 0; j < 3; j++) {
                // avoid round off error when possible
                if (plane.getNormal()[j] == 1.0f) {
                    mid[j] = plane.getDistance();
                }
                else if (plane.getNormal()[j] == -1.0f) {
                    mid[j] = -plane.getDistance();
                }
                else {
                    mid[j] = p2[j] + dot * (p1[j] - p2[j]);
                }
            }

            mid.s = p2.s + dot * (p1.s - p2.s);
            mid.t = p2.t + dot * (p1.t - p2.t);
        }

        f->pPoints_[f->numPoints_] = mid;
        f->numPoints_++;
        b->pPoints_[b->numPoints_] = mid;
        b->numPoints_++;
    }

    if (f->numPoints_ > maxpts || b->numPoints_ > maxpts) {
        X_WARNING("XWinding", "points exceeded estimate");
    }

    return PlaneSide::CROSS;
}

template<class Allocator>
void XWindingT<Allocator>::AddToConvexHull(const XWindingT* pWinding, const Vec3f& normal, const float epsilon)
{
    int i, j, k;
    Vec3f dir;
    float d;
    int maxPts;
    Vec3f* hullDirs;
    bool* hullSide;
    bool outside;
    int numNewHullPoints;
    Vec5f* newHullPoints;

    if (!pWinding) {
        return;
    }

    maxPts = numPoints_ + pWinding->numPoints_;

    EnsureAlloced(maxPts, true);

    newHullPoints = (Vec5f*)alloca16(maxPts * sizeof(Vec5f));
    hullDirs = (Vec3f*)alloca16(maxPts * sizeof(Vec3f));
    hullSide = (bool*)alloca16(maxPts * sizeof(bool));

    for (i = 0; i < pWinding->numPoints_; i++) {
        const Vec5f& p1 = pWinding->pPoints_[i];

        // calculate hull edge vectors
        for (j = 0; j < numPoints_; j++) {
            dir = pPoints_[(j + 1) % numPoints_].asVec3() - pPoints_[j].asVec3();
            dir.normalize();
            hullDirs[j] = normal.cross(dir);
        }

        // calculate side for each hull edge
        outside = false;
        for (j = 0; j < numPoints_; j++) {
            dir = p1.asVec3() - pPoints_[j].asVec3();
            d = dir * hullDirs[j];
            if (d >= epsilon) {
                outside = true;
            }
            if (d >= -epsilon) {
                hullSide[j] = true;
            }
            else {
                hullSide[j] = false;
            }
        }

        // if the point is effectively inside, do nothing
        if (!outside) {
            continue;
        }

        // find the back side to front side transition
        for (j = 0; j < numPoints_; j++) {
            if (!hullSide[j] && hullSide[(j + 1) % numPoints_]) {
                break;
            }
        }
        if (j >= numPoints_) {
            continue;
        }

        // insert the point here
        newHullPoints[0] = p1;
        numNewHullPoints = 1;

        // copy over all points that aren't double fronts
        j = (j + 1) % numPoints_;
        for (k = 0; k < numPoints_; k++) {
            if (hullSide[(j + k) % numPoints_] && hullSide[(j + k + 1) % numPoints_]) {
                continue;
            }
            newHullPoints[numNewHullPoints] = pPoints_[(j + k + 1) % numPoints_];
            numNewHullPoints++;
        }

        numPoints_ = numNewHullPoints;
        memcpy(pPoints_, newHullPoints, numNewHullPoints * sizeof(Vec5f));
    }
}

template<class Allocator>
void XWindingT<Allocator>::AddToConvexHull(const Vec3f& point, const Vec3f& normal, const float epsilon)
{
    int j, k, numHullPoints;
    Vec3f dir;

    switch (numPoints_) {
        case 0: {
            pPoints_[0] = point;
            numPoints_++;
            return;
        }
        case 1: {
            // don't add the same point second
            if (pPoints_[0].asVec3().compare(point, epsilon)) {
                return;
            }
            pPoints_[1].asVec3() = point;
            numPoints_++;
            return;
        }
        case 2: {
            // don't add a point if it already exists
            if (pPoints_[0].asVec3().compare(point, epsilon) || pPoints_[1].asVec3().compare(point, epsilon)) {
                return;
            }
            // if only two points make sure we have the right ordering according to the normal
            dir = point - pPoints_[0].asVec3();
            dir = dir.cross(pPoints_[1].asVec3() - pPoints_[0].asVec3());
            if (dir[0] == 0.0f && dir[1] == 0.0f && dir[2] == 0.0f) {
                // points don't make a plane
                return;
            }
            if (dir * normal > 0.0f) {
                pPoints_[2].asVec3() = point;
            }
            else {
                pPoints_[2] = pPoints_[1];
                pPoints_[1].asVec3() = point;
            }
            numPoints_++;
            return;
        }
    }

    Vec3f* hullDirs = (Vec3f*)alloca16(numPoints_ * sizeof(Vec3f));
    bool* hullSide = (bool*)alloca16(numPoints_ * sizeof(bool));

    // calculate hull edge vectors
    for (j = 0; j < numPoints_; j++) {
        dir = pPoints_[(j + 1) % numPoints_].asVec3() - pPoints_[j].asVec3();
        hullDirs[j] = normal.cross(dir);
    }

    // calculate side for each hull edge
    bool outside = false;
    for (j = 0; j < numPoints_; j++) {
        dir = point - pPoints_[j].asVec3();
        float d = dir * hullDirs[j];
        if (d >= epsilon) {
            outside = true;
        }
        if (d >= -epsilon) {
            hullSide[j] = true;
        }
        else {
            hullSide[j] = false;
        }
    }

    // if the point is effectively inside, do nothing
    if (!outside) {
        return;
    }

    // find the back side to front side transition
    for (j = 0; j < numPoints_; j++) {
        if (!hullSide[j] && hullSide[(j + 1) % numPoints_]) {
            break;
        }
    }
    if (j >= numPoints_) {
        return;
    }

    Vec5f* hullPoints = reinterpret_cast<Vec5f*>(alloca16((numPoints_ + 1) * sizeof(Vec5f)));

    // insert the point here
    hullPoints[0] = point;
    numHullPoints = 1;

    // copy over all points that aren't double fronts
    j = (j + 1) % numPoints_;
    for (k = 0; k < numPoints_; k++) {
        if (hullSide[(j + k) % numPoints_] && hullSide[(j + k + 1) % numPoints_]) {
            continue;
        }
        hullPoints[numHullPoints] = pPoints_[(j + k + 1) % numPoints_];
        numHullPoints++;
    }

    EnsureAlloced(numHullPoints, false);
    numPoints_ = numHullPoints;
    memcpy(pPoints_, hullPoints, numHullPoints * sizeof(Vec3f));
}

// ----------------------------------------------------------------------

template<class Allocator>
float XWindingT<Allocator>::TriangleArea(const Vec3f& a, const Vec3f& b, const Vec3f& c)
{
    Vec3f v1, v2;
    Vec3f cross;

    v1 = b - a;
    v2 = c - a;
    cross = v1.cross(v2);
    return 0.5f * cross.length();
}

template<class Allocator>
void XWindingT<Allocator>::NormalVectors(const Vec3f& vec, Vec3f& left, Vec3f& down)
{
    float d;

    auto InvSqrt = [](float x) -> float {
        return (x > core::FloatUtil::FLT_SMALLEST_NON_DENORMAL) ? math<float>::square(1.0f / x) : INFINITY;
    };

    d = vec.x * vec.x + vec.y * vec.y;
    if (!d) {
        left[0] = 1;
        left[1] = 0;
        left[2] = 0;
    }
    else {
        d = InvSqrt(d);
        left[0] = -vec.y * d;
        left[1] = vec.x * d;
        left[2] = 0;
    }
    down = left.cross(vec);
}

// ----------------------------------------------------------------------

template<class Allocator>
bool XWindingT<Allocator>::SSave(core::XFile* pFile) const
{
    X_ASSERT_NOT_NULL(pFile);

    int32_t num = safe_static_cast<int32_t>(numPoints_);

    if (!pFile->writeObj(num)) {
        X_ERROR("Winding", "Failed to save winding to file");
        return false;
    }

    if (!pFile->writeObj(pPoints_, num)) {
        X_ERROR("Winding", "Failed to save winding to file");
        return false;
    }

    return true;
}

template<class Allocator>
bool XWindingT<Allocator>::SLoad(core::XFile* pFile)
{
    X_ASSERT_NOT_NULL(pFile);

    int32_t num;
    if (!pFile->readObj(num)) {
        X_ERROR("Winding", "Failed to load winding from file");
        return false;
    }

    if (num) {
        EnsureAlloced(num, false);
        if (pFile->readObj(pPoints_, num) != (num * sizeof(pPoints_[0]))) {
            X_ERROR("Winding", "Failed to load winding point from file");
            clear();
            return false;
        }
    }

    numPoints_ = num;
    return true;
}

template<class Allocator>
X_INLINE const Vec5f& XWindingT<Allocator>::operator[](const size_t idx) const
{
    X_ASSERT(static_cast<int32_t>(idx) < numPoints_, "index out of range")
    (idx, getNumPoints());
    return pPoints_[idx];
}

template<class Allocator>
X_INLINE Vec5f& XWindingT<Allocator>::operator[](const size_t idx)
{
    X_ASSERT(static_cast<int32_t>(idx) < numPoints_, "index out of range")
    (idx, getNumPoints());
    return pPoints_[idx];
}

template<class Allocator>
X_INLINE const Vec5f& XWindingT<Allocator>::at(size_t idx) const
{
    X_ASSERT(static_cast<int32_t>(idx) < numPoints_, "index out of range")
    (idx, getNumPoints());
    return pPoints_[idx];
}

template<class Allocator>
X_INLINE Vec5f& XWindingT<Allocator>::at(size_t idx)
{
    X_ASSERT(static_cast<int32_t>(idx) < numPoints_, "index out of range")
    (idx, getNumPoints());
    return pPoints_[idx];
}

// add a point to the end of the winding point array
template<class Allocator>
X_INLINE XWindingT<Allocator>& XWindingT<Allocator>::operator+=(const Vec5f& v)
{
    addPoint(v);
    return *this;
}

template<class Allocator>
X_INLINE XWindingT<Allocator>& XWindingT<Allocator>::operator+=(const Vec3f& v)
{
    addPoint(v);
    return *this;
}

template<class Allocator>
X_INLINE void XWindingT<Allocator>::addPoint(const Vec5f& v)
{
    EnsureAlloced(numPoints_ + 1, true);
    pPoints_[numPoints_] = v;
    numPoints_++;
}

template<class Allocator>
X_INLINE void XWindingT<Allocator>::addPoint(const Vec3f& v)
{
    EnsureAlloced(numPoints_ + 1, true);
    pPoints_[numPoints_] = v;
    numPoints_++;
}

template<class Allocator>
X_INLINE size_t XWindingT<Allocator>::getNumPoints(void) const
{
    return numPoints_;
}

template<class Allocator>
X_INLINE size_t XWindingT<Allocator>::getAllocatedSize(void) const
{
    return allocedSize_;
}

template<class Allocator>
inline typename XWindingT<Allocator>::TypePtr XWindingT<Allocator>::ptr(void)
{
    return pPoints_;
}

template<class Allocator>
inline typename XWindingT<Allocator>::ConstTypePtr XWindingT<Allocator>::ptr(void) const
{
    return pPoints_;
}

template<class Allocator>
inline typename XWindingT<Allocator>::TypePtr XWindingT<Allocator>::data(void)
{
    return pPoints_;
}

template<class Allocator>
inline typename XWindingT<Allocator>::ConstTypePtr XWindingT<Allocator>::data(void) const
{
    return pPoints_;
}

template<class Allocator>
inline typename XWindingT<Allocator>::Iterator XWindingT<Allocator>::begin(void)
{
    return pPoints_;
}

template<class Allocator>
inline typename XWindingT<Allocator>::ConstIterator XWindingT<Allocator>::begin(void) const
{
    return pPoints_;
}

template<class Allocator>
inline typename XWindingT<Allocator>::Iterator XWindingT<Allocator>::end(void)
{
    return pPoints_ + numPoints_;
}

template<class Allocator>
inline typename XWindingT<Allocator>::ConstIterator XWindingT<Allocator>::end(void) const
{
    return pPoints_ + numPoints_;
}

template<class Allocator>
X_INLINE void XWindingT<Allocator>::EnsureAlloced(size_t n, bool keep)
{
    int32_t num = safe_static_cast<int32_t, size_t>(n);
    if (num > allocedSize_) {
        ReAllocate(num, keep);
    }
}

template<class Allocator>
X_INLINE void XWindingT<Allocator>::ReAllocate(int32_t num, bool keep)
{
    Vec5f* pOldPoints = pPoints_;

    num = core::bitUtil::RoundUpToMultiple(num, 4);
    pPoints_ = allocator_.alloc(num);

    if (pOldPoints) {
        if (keep) {
            memcpy(pPoints_, pOldPoints, numPoints_ * sizeof(pPoints_[0]));
        }
        allocator_.free(pOldPoints);
    }

    allocedSize_ = num;
}