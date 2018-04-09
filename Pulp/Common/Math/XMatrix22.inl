
template<typename T>
Matrix22<T>::Matrix22()
{
    setToIdentity();
}

template<typename T>
Matrix22<T>::Matrix22(T s)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] = s;
    }
}

template<typename T>
Matrix22<T>::Matrix22(T d0, T d1, T d2, T d3, bool srcIsRowMajor)
{
    set(d0, d1,
        d2, d3, srcIsRowMajor);
}

template<typename T>
Matrix22<T>::Matrix22(const Vec2<T>& vx, const Vec2<T>& vy)
{
    m00 = vx.x;
    m01 = vy.x;
    m10 = vx.y;
    m11 = vy.y;
}


template<typename T>
Matrix22<T>::Matrix22(const Matrix22<T>& src)
{
    std::memcpy(m, src.m, MEM_LEN);
}

template<typename T>
template<typename FromT>
Matrix22<T>::Matrix22(const Matrix22<FromT>& src)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] = static_cast<T>(src.m[i]);
    }
}


template<typename T>
Matrix22<T>& Matrix22<T>::operator=(const T& rhs)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] = rhs;
    }
    return *this;
}

template<typename T>
Matrix22<T>& Matrix22<T>::operator=(const Matrix22<T>& rhs)
{
    std::memcpy(m, rhs.m, MEM_LEN);
    return *this;
}

template<typename T>
template<typename FromT>
Matrix22<T>& Matrix22<T>::operator=(const Matrix22<FromT>& rhs)
{
    for (int i = 0; i < DIM_SQ; i++) {
        m[i] = static_cast<T>(rhs.m[i]);
    }
    return *this;
}

template<typename T>
Matrix22<T>::operator T*()
{
    return (T*)m;
}

template<typename T>
Matrix22<T>::operator const T*() const
{
    return (const T*)m;
}

template<typename T>
bool Matrix22<T>::equalCompare(const Matrix22<T>& rhs, T epsilon) const
{
    for (int i = 0; i < DIM_SQ; ++i) {
        T diff = fabs(m[i] - rhs.m[i]);
        if (diff >= epsilon) {
            return false;
        }
    }
    return true;
}

template<typename T>
bool Matrix22<T>::operator==(const Matrix22<T>& rhs) const
{
    return equalCompare(rhs, EPSILON);
}

template<typename T>
bool Matrix22<T>::operator!=(const Matrix22<T>& rhs) const
{
    return !(*this == rhs);
}

template<typename T>
Matrix22<T>& Matrix22<T>::operator*=(const Matrix22<T>& rhs)
{
    Matrix22<T> mat;

    mat.m[0] = m[0] * rhs.m[0] + m[2] * rhs.m[1];
    mat.m[1] = m[1] * rhs.m[0] + m[3] * rhs.m[1];

    mat.m[2] = m[0] * rhs.m[2] + m[2] * rhs.m[3];
    mat.m[3] = m[1] * rhs.m[2] + m[3] * rhs.m[3];

    *this = mat;

    return *this;
}

template<typename T>
Matrix22<T>& Matrix22<T>::operator+=(const Matrix22<T>& rhs)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] += rhs.m[i];
    }
    return *this;
}

template<typename T>
Matrix22<T>& Matrix22<T>::operator-=(const Matrix22<T>& rhs)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] -= rhs.m[i];
    }
    return *this;
}

template<typename T>
Matrix22<T>& Matrix22<T>::operator*=(T s)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] *= s;
    }
    return *this;
}

template<typename T>
Matrix22<T>& Matrix22<T>::operator/=(T s)
{
    T invS = (T)1 / s;
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] *= invS;
    }
    return *this;
}

template<typename T>
Matrix22<T>& Matrix22<T>::operator+=(T s)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] += s;
    }
    return *this;
}

template<typename T>
Matrix22<T>& Matrix22<T>::operator-=(T s)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] -= s;
    }
    return *this;
}

template<typename T>
const Matrix22<T> Matrix22<T>::operator*(const Matrix22<T>& rhs) const
{
    Matrix22<T> ret;

    ret.m[0] = m[0] * rhs.m[0] + m[2] * rhs.m[1];
    ret.m[1] = m[1] * rhs.m[0] + m[3] * rhs.m[1];

    ret.m[2] = m[0] * rhs.m[2] + m[2] * rhs.m[3];
    ret.m[3] = m[1] * rhs.m[2] + m[3] * rhs.m[3];

    return ret;
}

template<typename T>
const Matrix22<T> Matrix22<T>::operator+(const Matrix22<T>& rhs) const
{
    Matrix22<T> ret;
    for (int i = 0; i < DIM_SQ; ++i) {
        ret.m[i] = m[i] + rhs.m[i];
    }
    return ret;
}

template<typename T>
const Matrix22<T> Matrix22<T>::operator-(const Matrix22<T>& rhs) const
{
    Matrix22<T> ret;
    for (int i = 0; i < DIM_SQ; ++i) {
        ret.m[i] = m[i] - rhs.m[i];
    }
    return ret;
}

template<typename T>
const Vec2<T> Matrix22<T>::operator*(const Vec2<T>& rhs) const
{
    return Vec2<T>(
        m[0] * rhs.x + m[2] * rhs.y,
        m[1] * rhs.x + m[3] * rhs.y);
}

template<typename T>
const Matrix22<T> Matrix22<T>::operator*(T rhs) const
{
    Matrix22<T> ret;
    for (int i = 0; i < DIM_SQ; ++i) {
        ret.m[i] = m[i] * rhs;
    }
    return ret;
}

template<typename T>
const Matrix22<T> Matrix22<T>::operator/(T rhs) const
{
    Matrix22<T> ret;
    T s = (T)1 / rhs;
    for (int i = 0; i < DIM_SQ; ++i) {
        ret.m[i] = m[i] * s;
    }
    return ret;
}

template<typename T>
const Matrix22<T> Matrix22<T>::operator+(T rhs) const
{
    Matrix22<T> ret;
    for (int i = 0; i < DIM_SQ; ++i) {
        ret.m[i] = m[i] + rhs;
    }
    return ret;
}

template<typename T>
const Matrix22<T> Matrix22<T>::operator-(T rhs) const
{
    Matrix22<T> ret;
    for (int i = 0; i < DIM_SQ; ++i) {
        ret.m[i] = m[i] - rhs;
    }
    return ret;
}

template<typename T>
T& Matrix22<T>::at(int row, int col)
{
    X_ASSERT(row >= 0 && row < DIM, "row out of range")(DIM, row);
    X_ASSERT(col >= 0 && col < DIM, "col out of range")(DIM, col);
    return m[col * DIM + row];
}

template<typename T>
const T& Matrix22<T>::at(int row, int col) const
{
    X_ASSERT(row >= 0 && row < DIM, "row out of range")(DIM, row);
    X_ASSERT(col >= 0 && col < DIM, "col out of range")(DIM, col);
    return m[col * DIM + row];
}

template<typename T>
void Matrix22<T>::set(T d0, T d1, T d2, T d3, bool srcIsRowMajor)
{
    if (srcIsRowMajor) {
        m[0] = d0;
        m[2] = d1;
        m[1] = d2;
        m[3] = d3;
    }
    else {
        m[0] = d0;
        m[2] = d2;
        m[1] = d1;
        m[3] = d3;
    }
}

template<typename T>
Vec2<T> Matrix22<T>::getColumn(int col) const
{
    size_t i = col * DIM;
    return Vec2<T>(
        m[i + 0],
        m[i + 1]);
}

template<typename T>
void Matrix22<T>::setColumn(int col, const Vec2<T>& v)
{
    size_t i = col * DIM;
    m[i + 0] = v.x;
    m[i + 1] = v.y;
}

template<typename T>
Vec2<T> Matrix22<T>::getRow(int row) const
{
    return Vec2<T>(
        m[row + 0],
        m[row + 2]);
}

template<typename T>
void Matrix22<T>::setRow(int row, const Vec2<T>& v)
{
    m[row + 0] = v.x;
    m[row + 2] = v.y;
}

template<typename T>
void Matrix22<T>::getColumns(Vec2<T>* c0, Vec2<T>* c1) const
{
    *c0 = getColumn(0);
    *c1 = getColumn(1);
}

template<typename T>
void Matrix22<T>::setColumns(const Vec2<T>& c0, const Vec2<T>& c1)
{
    setColumn(0, c0);
    setColumn(1, c1);
}

template<typename T>
void Matrix22<T>::getRows(Vec2<T>* r0, Vec2<T>* r1) const
{
    *r0 = getRow(0);
    *r1 = getRow(1);
}

template<typename T>
void Matrix22<T>::setRows(const Vec2<T>& r0, const Vec2<T>& r1)
{
    setRow(0, r0);
    setRow(1, r1);
}

template<typename T>
void Matrix22<T>::setToNull()
{
    std::memset(m, 0, MEM_LEN);
}

template<typename T>
void Matrix22<T>::setToIdentity()
{
    m00 = 1;
    m01 = 0;
    m10 = 0;
    m11 = 1;
}

template<typename T>
T Matrix22<T>::determinant() const
{
    T det = m[0] * m[3] - m[1] * m[2];
    return det;
}

template<typename T>
T Matrix22<T>::trace() const
{
    return m00 + m11;
}

template<typename T>
Matrix22<T> Matrix22<T>::diagonal() const
{
    Matrix22 ret;
    ret.m00 = m00;
    ret.m01 = 0;
    ret.m10 = 0;
    ret.m11 = m11;
    return ret;
}

template<typename T>
Matrix22<T> Matrix22<T>::lowerTriangular() const
{
    Matrix22 ret;
    ret.m00 = m00;
    ret.m01 = 0;
    ret.m10 = m10;
    ret.m11 = m11;
    return ret;
}

template<typename T>
Matrix22<T> Matrix22<T>::upperTriangular() const
{
    Matrix22 ret;
    ret.m00 = m00;
    ret.m01 = m01;
    ret.m10 = 0;
    ret.m11 = m11;
    return ret;
}

template<typename T>
void Matrix22<T>::transpose()
{
    // 0 1
    // 1 0
    core::Swap(m01, m10);
}

template<typename T>
Matrix22<T> Matrix22<T>::transposed() const
{
    return Matrix22<T>(
        m[0], m[2],
        m[1], m[3]);
}

template<typename T>
void Matrix22<T>::invert(T epsilon)
{
    *this = inverted(epsilon);
}

template<typename T>
Matrix22<T> Matrix22<T>::inverted(T epsilon) const
{
    Matrix22<T> inv((T)0);

    T det = m[0] * m[3] - m[1] * m[2];

    if (fabs(det) > epsilon) {
        T invDet = (T)1 / det;
        inv.m[0] = m[3] * invDet;
        inv.m[1] = -m[1] * invDet;
        inv.m[2] = -m[2] * invDet;
        inv.m[3] = m[0] * invDet;
    }

    return inv;
}

template<typename T>
Vec2<T> Matrix22<T>::preMultiply(const Vec2<T>& v) const
{
    return Vec2<T>(
        v.x * m00 + v.y * m10,
        v.x * m01 + v.y * m11);
}

template<typename T>
Vec2<T> Matrix22<T>::postMultiply(const Vec2<T>& v) const
{
    return Vec2<T>(
        m00 * v.x + m01 * v.y,
        m10 * v.x + m11 * v.y);
}

template<typename T>
Vec2<T> Matrix22<T>::transformVec(const Vec2<T>& v) const
{
    return postMultiply(v);
}

// rotate by radians (conceptually, rotate is before 'this')
template<typename T>
void Matrix22<T>::rotate(T radians)
{
    Matrix22 rot = createRotation(radians);
    Matrix22 mat = *this;
    *this = rot * mat;
}

// concatenate scale (conceptually, scale is before 'this')
template<typename T>
void Matrix22<T>::scale(T s)
{
    Matrix22 sc = createScale(s);
    Matrix22 mat = *this;
    *this = sc * mat;
}

template<typename T>
void Matrix22<T>::scale(const Vec2<T>& v)
{
    Matrix22 sc = createScale(v);
    Matrix22 mat = *this;
    *this = sc * mat;
}


template<typename T>
Matrix22<T> Matrix22<T>::invertTransform() const
{
    Matrix22<T> ret;

    // transpose rotation part
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            ret.at(j, i) = at(i, j);
        }
    }

    return ret;
}

template<typename T>
const char* Matrix22<T>::toString(Description& desc) const
{
    desc.setFmt("<%g,%g> - <%g,%g>", 
        m00, m01, 
        m10, m11);
    return desc.c_str();
}

// -------------------------------------------------------

template<typename T>
Matrix22<T> Matrix22<T>::identity()
{
    return Matrix22(1, 0, 0, 1);
}

template<typename T>
Matrix22<T> Matrix22<T>::one()
{
    return Matrix22((T)1);
}

template<typename T>
Matrix22<T> Matrix22<T>::zero()
{
    return Matrix22((T)0);
}

template<typename T>
Matrix22<T> Matrix22<T>::createRotation(T radians)
{
    Matrix22<T> ret;
    T ac = cos(radians);
    T as = sin(radians);
    ret.m00 = ac;
    ret.m01 = as;
    ret.m10 = -as;
    ret.m11 = ac;
    return ret;
}

template<typename T>
Matrix22<T> Matrix22<T>::createScale(T s)
{
    Matrix22<T> ret;
    ret.m00 = s;
    ret.m01 = 0;
    ret.m10 = 0;
    ret.m11 = s;
    return ret;
}

template<typename T>
Matrix22<T> Matrix22<T>::createScale(const Vec2<T>& v)
{
    Matrix22<T> ret;
    ret.m00 = v.x;
    ret.m01 = 0;
    ret.m10 = 0;
    ret.m11 = v.y;
    return ret;
}
