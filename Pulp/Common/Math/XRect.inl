

template<typename T>
RectT<T>::RectT() :
    x1(0),
    y1(0),
    x2(0),
    y2(0)
{
}

template<typename T>
RectT<T>::RectT(T aX1, T aY1, T aX2, T aY2)
{
    set(aX1, aY1, aX2, aY2);
}

template<typename T>
RectT<T>::RectT(const Vec2<T>& v1, const Vec2<T>& v2)
{
    set(v1.x, v1.y, v2.x, v2.y);
}


template<typename T>
void RectT<T>::set(T aX1, T aY1, T aX2, T aY2)
{
    x1 = aX1;
    y1 = aY1;
    x2 = aX2;
    y2 = aY2;
}

template<typename T>
T RectT<T>::getWidth(void) const
{
    return x2 - x1;
}

template<typename T>
T RectT<T>::getHeight(void) const
{
    return y2 - y1;
}

template<typename T>
T RectT<T>::getAspectRatio(void) const
{
    return getWidth() / getHeight();
}

template<typename T>
T RectT<T>::calcArea(void) const
{
    return getWidth() * getHeight();
}

template<typename T>
void RectT<T>::canonicalize(void)
{
    if (x1 > x2) {
        T temp = x1;
        x1 = x2;
        x2 = temp;
    }

    if (y1 > y2) {
        T temp = y1;
        y1 = y2;
        y2 = temp;
    }
}

template<typename T>
RectT<T> RectT<T>::canonicalized(void) const
{
    RectT<T> result(*this);
    result.canonicalize();
    return result;
}

template<typename T>
void RectT<T>::clipBy(const RectT& clip)
{
    if (x1 < clip.x1) {
        x1 = clip.x1;
    }
    if (x2 < clip.x1) {
        x2 = clip.x1;
    }
    if (x1 > clip.x2) {
        x1 = clip.x2;
    }
    if (x2 > clip.x2) {
        x2 = clip.x2;
    }

    if (y1 < clip.y1) {
        y1 = clip.y1;
    }
    if (y2 < clip.y1) {
        y2 = clip.y1;
    }
    if (y1 > clip.y2) {
        y1 = clip.y2;
    }
    if (y2 > clip.y2) {
        y2 = clip.y2;
    }
}

template<typename T>
RectT<T> RectT<T>::getClipBy(const RectT& clip) const
{
    RectT<T> result(*this);
    result.clipBy(RectT<T>(clip));
    return result;
}

template<typename T>
void RectT<T>::offset(const Vec2<T>& offset)
{
    x1 += offset.x;
    x2 += offset.x;
    y1 += offset.y;
    y2 += offset.y;
}

template<typename T>
RectT<T> RectT<T>::getOffset(const Vec2<T>& off) const
{
    RectT<T> result(*this);
    result.offset(off);
    return result;
}

template<typename T>
void RectT<T>::inflate(const Vec2<T>& amount)
{
    x1 -= amount.x;
    x2 += amount.x;
    y1 -= amount.y; // assume canonical rect has y1 < y2
    y2 += amount.y;
}

template<typename T>
RectT<T> RectT<T>::inflated(const Vec2<T>& amount) const
{
    RectT<T> result(*this);
    result.inflate(amount);
    return result;
}

template<typename T>
void RectT<T>::offsetCenterTo(const Vec2<T>& center)
{
    offset(center - getCenter());
}

template<typename T>
void RectT<T>::scaleCentered(const Vec2<T>& scale)
{
    T halfWidth = getWidth() * scale.x / 2.0f;
    T halfHeight = getHeight() * scale.y / 2.0f;
    const Vec2<T> center(getCenter());
    x1 = center.x - halfWidth;
    x2 = center.x + halfWidth;
    y1 = center.y - halfHeight;
    y2 = center.y + halfHeight;
}

template<typename T>
void RectT<T>::scaleCentered(T scale)
{
    T halfWidth = getWidth() * scale / 2;
    T halfHeight = getHeight() * scale / 2;
    const Vec2<T> center(getCenter());
    x1 = center.x - halfWidth;
    x2 = center.x + halfWidth;
    y1 = center.y - halfHeight;
    y2 = center.y + halfHeight;
}

template<typename T>
RectT<T> RectT<T>::scaledCentered(T scale) const
{
    T halfWidth = getWidth() * scale / 2;
    T halfHeight = getHeight() * scale / 2;
    const Vec2<T> center(getCenter());
    return RectT<T>(center.x - halfWidth, center.y - halfHeight, center.x + halfWidth, center.y + halfHeight);
}

template<typename T>
void RectT<T>::scale(T s)
{
    x1 *= s;
    x2 *= s;
    y1 *= s;
    y2 *= s;
}

template<typename T>
void RectT<T>::scale(const Vec2<T>& scale)
{
    x1 *= scale.x;
    y1 *= scale.y;
    x2 *= scale.x;
    y2 *= scale.y;
}

template<typename T>
RectT<T> RectT<T>::scaled(T s) const
{
    return RectT<T>(x1 * s, y1 * s, x2 * s, y2 * s);
}

template<typename T>
RectT<T> RectT<T>::scaled(const Vec2<T>& scale) const
{
    return RectT<T>(x1 * scale.x, y1 * scale.y, x2 * scale.x, y2 * scale.y);
}

template<typename T>
template<typename Y>
bool RectT<T>::contains(const Vec2<Y>& pt) const
{
    return (pt.x >= x1) && (pt.x <= x2) && (pt.y >= y1) && (pt.y <= y2);
}

template<typename T>
bool RectT<T>::contains(const RectT<T>& rect) const
{
    return (rect.x1 >= x1 && rect.y1 >= y1 && rect.x2 <= x2 && rect.y2 <= y2);
}

template<typename T>
bool RectT<T>::intersects(const RectT<T>& rect) const
{
    if ((x1 > rect.x2) || (x2 < rect.x1) || (y1 > rect.y2) || (y2 < rect.y1)) {
        return false;
    }
    
    return true;
}

template<typename T>
T RectT<T>::distance(const Vec2<T>& pt) const
{
    T squaredDistance = 0;
    if (pt.x < x1) {
        squaredDistance += (x1 - pt.x) * (x1 - pt.x);
    }
    else if (pt.x > x2) {
        squaredDistance += (pt.x - x2) * (pt.x - x2);
    }
    if (pt.y < y1) {
        squaredDistance += (y1 - pt.y) * (y1 - pt.y);
    }
    else if (pt.y > y2) {
        squaredDistance += (pt.y - y2) * (pt.y - y2);
    }

    if (squaredDistance > 0) {
        return math<T>::sqrt(squaredDistance);
    }
    else {
        return 0;
    }
}

template<typename T>
T RectT<T>::distanceSquared(const Vec2<T>& pt) const
{
    T squaredDistance = 0;
    if (pt.x < x1) {
        squaredDistance += (x1 - pt.x) * (x1 - pt.x);
    }
    else if (pt.x > x2) {
        squaredDistance += (pt.x - x2) * (pt.x - x2);
    }
    if (pt.y < y1) {
        squaredDistance += (y1 - pt.y) * (y1 - pt.y);
    }
    else if (pt.y > y2) {
        squaredDistance += (pt.y - y2) * (pt.y - y2);
    }

    return squaredDistance;
}

template<typename T>
Vec2<T> RectT<T>::closestPoint(const Vec2<T>& pt) const
{
    Vec2<T> result = pt;
    if (pt.x < x1) {
        result.x = x1;
    }
    else if (pt.x > x2) {
        result.x = x2;
    }
    if (pt.y < y1) {
        result.y = y1;
    }
    else if (pt.y > y2) {
        result.y = y2;
    }
    return result;
}

template<typename T>
T RectT<T>::getX1(void) const
{
    return x1;
}

template<typename T>
T RectT<T>::getY1(void) const
{
    return y1;
}

template<typename T>
T RectT<T>::getX2(void) const
{
    return x2;
}

template<typename T>
T RectT<T>::getY2(void) const
{
    return y2;
}

template<typename T>
Vec2<T> RectT<T>::getUpperLeft(void) const
{
    return Vec2<T>(x1, y1);
};

template<typename T>
Vec2<T> RectT<T>::getUpperRight(void) const
{
    return Vec2<T>(x2, y1);
};

template<typename T>
Vec2<T> RectT<T>::getLowerRight(void) const
{
    return Vec2<T>(x2, y2);
};

template<typename T>
Vec2<T> RectT<T>::getLowerLeft(void) const
{
    return Vec2<T>(x1, y2);
};

template<typename T>
Vec2<T> RectT<T>::getCenter(void) const
{
    return Vec2<T>((x1 + x2) / 2, (y1 + y2) / 2);
}

template<typename T>
Vec2<T> RectT<T>::getSize(void) const
{
    return Vec2<T>(x2 - x1, y2 - y1);
}

template<typename T>
RectT<T> RectT<T>::getCenteredFit(const RectT<T>& other, bool expand) const
{
    RectT<T> result = *this;
    result.offset(other.getCenter() - result.getCenter());

    bool isInside = ((result.getWidth() < other.getWidth()) && (result.getHeight() < other.getHeight()));
    if (expand || (!isInside)) { // need to do some scaling
        T aspectAspect = result.getAspectRatio() / other.getAspectRatio();
        if (aspectAspect >= 1.0f) { // result is proportionally wider so we need to fit its x-axis
            T scaleBy = other.getWidth() / result.getWidth();
            result.scaleCentered(scaleBy);
        }
        else { // result is proportionally wider so we need to fit its y-axis
            T scaleBy = other.getHeight() / result.getHeight();
            result.scaleCentered(scaleBy);
        }
    }

    return result;
}


template<typename T>
void RectT<T>::include(const Vec2<T>& point)
{
    if (x1 > point.x) {
        x1 = point.x;
    }
    if (x2 < point.x) {
        x2 = point.x;
    }
    if (y1 > point.y) {
        y1 = point.y;
    }
    if (y2 < point.y) {
        y2 = point.y;
    }
}

template<typename T>
void RectT<T>::include(const RectT<T>& rect)
{
    include(Vec2<T>(rect.x1, rect.y1));
    include(Vec2<T>(rect.x2, rect.y2));
}

template<typename T>
RectT<T>& RectT<T>::Align(const RectT& other, AlignmentFlags alignment)
{
    Vec2<T> pos = getUpperLeft();
    Vec2<T> posOth = other.getUpperLeft();

    if (alignment.IsSet(Alignment::LEFT_ALIGN)) {
        pos.x = posOth.x;
    }
    else if (alignment.IsSet(Alignment::LEFT_DOCK)) {
        pos.x = posOth.x - getWidth();
    }
    else if (alignment.IsSet(Alignment::RIGHT_ALIGN)) {
        pos.x = posOth.x + other.getWidth() - getWidth();
    }
    else if (alignment.IsSet(Alignment::RIGHT_DOCK)) {
        pos.x = posOth.x + other.getWidth();
    }

    if (alignment.IsSet(Alignment::TOP_ALIGN)) {
        pos.y = posOth.y;
    }
    else if (alignment.IsSet(Alignment::TOP_DOCK)) {
        pos.y = posOth.y - getHeight();
    }
    else if (alignment.IsSet(Alignment::BOTTOM_ALIGN)) {
        pos.y = posOth.y + other.getHeight() - getHeight();
    }
    else if (alignment.IsSet(Alignment::BOTTOM_DOCK)) {
        pos.y = posOth.y + other.getHeight();
    }

    x1 = pos.x;
    y1 = pos.y;
    return *this;
}

template<typename T>
const RectT<T> RectT<T>::operator+(const Vec2<T>& o) const
{
    return this->getOffset(o);
}

template<typename T>
const RectT<T> RectT<T>::operator-(const Vec2<T>& o) const
{
    return this->getOffset(-o);
}

template<typename T>
const RectT<T> RectT<T>::operator*(T s) const
{
    return this->scaled(s);
}

template<typename T>
const RectT<T> RectT<T>::operator/(T s) const
{
    return this->scaled(((T)1) / s);
}

template<typename T>
const RectT<T> RectT<T>::operator+(const RectT<T>& rhs) const
{
    return RectT<T>(x1 + rhs.x1, y1 + rhs.y1, x2 + rhs.x2, y2 + rhs.y2);
}

template<typename T>
const RectT<T> RectT<T>::operator-(const RectT<T>& rhs) const
{
    return RectT<T>(x1 - rhs.x1, y1 - rhs.y1, x2 - rhs.x2, y2 - rhs.y2);
}

template<typename T>
RectT<T>& RectT<T>::operator+=(const Vec2<T>& o)
{
    offset(o);
    return *this;
}

template<typename T>
RectT<T>& RectT<T>::operator-=(const Vec2<T>& o)
{
    offset(-o);
    return *this;
}

template<typename T>
RectT<T>& RectT<T>::operator*=(T s)
{
    scale(s);
    return *this;
}

template<typename T>
RectT<T>& RectT<T>::operator/=(T s)
{
    scale(((T)1) / s);
    return *this;
}