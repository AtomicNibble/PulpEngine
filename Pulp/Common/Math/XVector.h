#pragma once


#ifndef _X_MATH_VEC_H_
#define _X_MATH_VEC_H_


// show me your vectors baby.

// distance tpye selection using specilisation.
template<typename T>
struct VECTRAIT {
	typedef float DIST;
};

template<>
struct VECTRAIT<double> {
	typedef double DIST;
};

template<>
struct VECTRAIT<int32_t> {
	typedef float DIST;
};


template<typename T>
class Vec2
{
public:
	T x, y;

	typedef T value_type;
	typedef typename VECTRAIT<T>::DIST	DIST;

	Vec2() : x(0), y(0) {}
	Vec2(T X, T Y) : x(X), y(Y) {}
	Vec2( const Vec2<T>& src ) : x(src.x), y(src.y) {}
	explicit Vec2(const T* pSrc) : x(pSrc[0]), y(pSrc[1]) {}

	X_INLINE void Set(T X, T Y) {
		x=X; y=Y;
	}
	X_INLINE void Set(const Vec2<T>& src) {
		x = src.x; y = src.y;
	}

	// kinky operators
	X_INLINE int operator!() const { return x == 0 && y == 0; }

	X_INLINE Vec2<T>& operator=(const Vec2<T>& oth)
	{
		x = oth.x;
		y = oth.y;
		return *this;
	}

	template<typename FromT>
	Vec2<T>& operator=(const Vec2<FromT>& oth)
	{
		x = static_cast<T>(oth.x);
		y = static_cast<T>(oth.y);
		return *this;
	}

	X_INLINE const T& operator[](size_t i) const {
		X_ASSERT(i >= 0 && i < 2, "out of range")(i);
		return (&x)[i];
	}

	X_INLINE T& operator[](size_t i) {
		X_ASSERT(i >= 0 && i < 2, "out of range")(i);
			return (&x)[i];
	}


	X_INLINE const Vec2<T>	operator+(const Vec2<T>& rhs) const { return Vec2<T>(x + rhs.x, y + rhs.y); }
	X_INLINE const Vec2<T>	operator-(const Vec2<T>& rhs) const { return Vec2<T>(x - rhs.x, y - rhs.y); }
	X_INLINE const Vec2<T>	operator*(const Vec2<T>& rhs) const { return Vec2<T>(x * rhs.x, y * rhs.y); }
	X_INLINE const Vec2<T>	operator/(const Vec2<T>& rhs) const { return Vec2<T>(x / rhs.x, y / rhs.y); }

	X_INLINE Vec2<T>&	operator+=(const Vec2<T>& rhs) { x += rhs.x; y += rhs.y; return *this; }
	X_INLINE Vec2<T>&	operator-=(const Vec2<T>& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
	X_INLINE Vec2<T>&	operator*=(const Vec2<T>& rhs)	{ x *= rhs.x; y *= rhs.y; return *this; }
	X_INLINE Vec2<T>&	operator/=(const Vec2<T>& rhs) { x /= rhs.x; y /= rhs.y; return *this; }
	X_INLINE const Vec2<T>	operator/(T rhs) const { return Vec2<T>(x / rhs, y / rhs); }
	X_INLINE Vec2<T>&	operator+=(T rhs)	{ x += rhs;	y += rhs; return *this; }
	X_INLINE Vec2<T>&	operator-=(T rhs) { x -= rhs; y -= rhs; return *this; }
	X_INLINE Vec2<T>&	operator*=(T rhs) { x *= rhs; y *= rhs; return *this; }
	X_INLINE Vec2<T>&	operator/=(T rhs) { x /= rhs; y /= rhs; return *this; }

	X_INLINE Vec2<T>	operator-() const { return Vec2<T>(-x, -y); }

	X_INLINE bool operator==(const Vec2<T>& oth) const {
		return x == oth.x && y == oth.y;
	}

	X_INLINE bool operator!=(const Vec2<T>& oth) const {
		return !(*this == oth);
	}

	X_INLINE bool compare(const Vec2<T>& oth, const T epsilon) const {
		return math<T>::abs(x - oth.x) <= epsilon &&
			math<T>::abs(y - oth.y) <= epsilon;
	}

	X_INLINE Vec2<T> abs(void) const {
		return Vec2<T>(math<T>::abs(x), math<T>::abs(y));
	}

	X_INLINE T dot(const Vec2<T>& oth) const {
		return x * oth.x + y * oth.y;
	}

	X_INLINE T cross(const Vec2<T>& oth) const {
		return x * oth.x - y * oth.y;
	}

	X_INLINE T distance(const Vec2<T> &rhs) const
	{
		return static_cast<T>((*this - rhs).length());
	}

	X_INLINE T distanceSquared(const Vec2<T> &rhs) const
	{
		return (*this - rhs).lengthSquared();
	}

	X_INLINE DIST length() const
	{
		return math<DIST>::sqrt((DIST)(x*x + y*y));
	}

	X_INLINE Vec2<T>& normalize()
	{
		DIST invS = 1 / length();
		x *= invS;
		y *= invS;
		return *this;
	}

	X_INLINE Vec2<T> normalized() const
	{
		DIST invS = 1 / length();
		return Vec2<T>(x * invS, y * invS);
	}

	// tests for zero-length
	X_INLINE Vec2<T>& normalizeSafe()
	{
		T s = lengthSquared();
		if (s > 0) {
			DIST invL = 1 / math<DIST>::sqrt(s);
			x *= invL;
			y *= invL;
		}
		return *this;
	}

	X_INLINE Vec2<T> normalizeSafe() const
	{
		T s = lengthSquared();
		if (s > 0) {
			DIST invL = 1 / math<DIST>::sqrt(s);
			return Vec2<T>(x * invL, y * invL);
		}
		else
			return Vec2<T>::zero();
	}

	X_INLINE void rotate(DIST radians)
	{
		T cosa = math<T>::cos(radians);
		T sina = math<T>::sin(radians);
		T rx = x * cosa - y * sina;
		y = x * sina + y * cosa;
		x = rx;
	}

	X_INLINE T lengthSquared() const
	{
		return x * x + y * y;
	}

	//! Limits the length of a Vec2 to \a maxLength, scaling it proportionally if necessary.
	X_INLINE void limit(DIST maxLength)
	{
		T lengthSquared = x * x + y * y;

		if ((lengthSquared > maxLength * maxLength) && (lengthSquared > 0)) {
			DIST ratio = maxLength / math<DIST>::sqrt(lengthSquared);
			x *= ratio;
			y *= ratio;
		}
	}

	//! Returns a copy of the Vec2 with its length limited to \a maxLength, scaling it proportionally if necessary.
	X_INLINE Vec2<T> limited(T maxLength) const
	{
		T lengthSquared = x * x + y * y;

		if ((lengthSquared > maxLength * maxLength) && (lengthSquared > 0)) {
			DIST ratio = maxLength / math<DIST>::sqrt(lengthSquared);
			return Vec2<T>(x * ratio, y * ratio);
		}
		else
			return *this;
	}

	X_INLINE void invert()
	{
		x = -x;
		y = -y;
	}

	X_INLINE Vec2<T> inverse() const
	{
		return Vec2<T>(-x, -y);
	}

	X_INLINE Vec2<T> lerp(T fact, const Vec2<T>& r) const
	{
		return (*this) + (r - (*this)) * fact;
	}

	X_INLINE void lerpEq(T fact, const Vec2<T> &rhs)
	{
		x = x + (rhs.x - x) * fact; y = y + (rhs.y - y) * fact;
	}

	static Vec2<T> zero()
	{
		return Vec2<T>(0, 0);
	}

	static Vec2<T> one()
	{
		return Vec2<T>(1, 1);
	}

	static Vec2<T> min()
	{
		return Vec2<T>(
			std::numeric_limits<T>::lowest(),
			std::numeric_limits<T>::lowest()
		);
	}
	static Vec2<T> max()
	{
		return Vec2<T>(
			std::numeric_limits<T>::max(),
			std::numeric_limits<T>::max()
		);
	}


	static Vec2<T> NaN()   { return Vec2<T>(math<T>::NaN(), math<T>::NaN()); }

	static Vec2<T> xAxis() { return Vec2<T>(1, 0); }
	static Vec2<T> yAxis() { return Vec2<T>(0, 1); }

	static Vec2<T> abs(const Vec2<T>& vec) {
		return vec.abs();
	}
};

template<typename T>
class Vec3
{
public:
	T x, y, z;

	typedef T value_type;
	typedef typename VECTRAIT<T>::DIST	DIST;

	Vec3() : x(0), y(0), z(0) {}
	Vec3(T X, T Y, T Z) : x(X), y(Y), z(Z) {}
	Vec3(const Vec3<T>& src) : x(src.x), y(src.y), z(src.z) {}
	template<typename TOth>
	explicit Vec3(const Vec3<TOth>& src) : 
		x(static_cast<T>(src.x)), 
		y(static_cast<T>(src.y)), 
		z(static_cast<T>(src.z)) {}
	explicit Vec3(const T* pSrc) : x(pSrc[0]), y(pSrc[1]), z(pSrc[2]) {}
	explicit Vec3(const T val) : x(val), y(val), z(val) {}
	explicit Vec3(const Vec2<T>& src) : x(src.x), y(src.y), z(0) {}


	X_INLINE void set(T X, T Y, T Z) {
		x = X; y = Y; z = Z;
	}
	X_INLINE void set(const Vec3<T>& src) {
		x = src.x; y = src.y; z = src.z;
	}

	// kinky operators
	X_INLINE int operator!() const { return x == 0 && y == 0 && z == 0; }

	X_INLINE Vec3<T>& operator=(const Vec3<T>& oth)
	{
		x = oth.x;
		y = oth.y;
		z = oth.z;
		return *this;
	}

	template<typename FromT>
	X_INLINE Vec3<T>& operator=(const Vec3<FromT>& oth)
	{
		x = static_cast<T>(oth.x);
		y = static_cast<T>(oth.y);
		z = static_cast<T>(oth.z);
		return *this;
	}

	X_INLINE const T& operator[](size_t i) const {
		X_ASSERT(i >= 0 && i < 3, "out of range")(i);
			return (&x)[i];
	}

	X_INLINE T& operator[](size_t i) {
		X_ASSERT(i >= 0 && i < 3, "out of range")(i);
			return (&x)[i];
	}


	X_INLINE const Vec3<T>	operator+(const Vec3<T>& rhs) const { return Vec3<T>(x + rhs.x, y + rhs.y, z + rhs.z); }
	X_INLINE const Vec3<T>	operator-(const Vec3<T>& rhs) const { return Vec3<T>(x - rhs.x, y - rhs.y, z - rhs.z); }
	X_INLINE const Vec3<T>	operator/(const Vec3<T>& rhs) const { return Vec3<T>(x / rhs.x, y / rhs.y, z / rhs.z); }
//	X_INLINE const Vec3<T>	operator*(const Vec3<T>& rhs) const { return Vec3<T>(x * rhs.x, y * rhs.y, z * rhs.z); }
	X_INLINE Vec3<T>		operator*(const float rhs) const { return Vec3<T>(x * rhs, y * rhs, z * rhs); }
	X_INLINE T				operator*(const Vec3<T>& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }

	X_INLINE Vec3<T>&	operator+=(const Vec3<T>& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
	X_INLINE Vec3<T>&	operator-=(const Vec3<T>& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
	X_INLINE Vec3<T>&	operator*=(const Vec3<T>& rhs)	{ x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }
	X_INLINE Vec3<T>&	operator/=(const Vec3<T>& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; return *this; }
	X_INLINE const Vec3<T>	operator/(T rhs) const { return Vec3<T>(x / rhs, y / rhs, z / rhs); }
	X_INLINE Vec3<T>&	operator+=(T rhs)	{ x += rhs;	y += rhs; z += rhs; return *this; }
	X_INLINE Vec3<T>&	operator-=(T rhs) { x -= rhs; y -= rhs; z -= rhs; return *this; }
	X_INLINE Vec3<T>&	operator*=(T rhs) { x *= rhs; y *= rhs; z *= rhs; return *this; }
	X_INLINE Vec3<T>&	operator/=(T rhs) { x /= rhs; y /= rhs; z /= rhs; return *this; }

	X_INLINE Vec3<T>	operator-() const { return Vec3<T>(-x, -y, -z); }

	X_INLINE bool operator==(const Vec3<T>& oth) const {
		return x == oth.x && y == oth.y && z == oth.z;
	}

	X_INLINE bool operator!=(const Vec3<T>& oth) const {
		return !(*this == oth);
	}

	X_INLINE bool compare(const Vec3<T>& oth, const T epsilon) const {		
		return math<T>::abs(x - oth.x) <= epsilon &&
			math<T>::abs(y - oth.y) <= epsilon &&
			math<T>::abs(z - oth.z) <= epsilon;
	}

	X_INLINE Vec3<T> abs(void) const {
		return Vec3<T>(math<T>::abs(x), math<T>::abs(y), math<T>::abs(z));
	}

	X_INLINE T dot(const Vec3<T>& oth) const {
		return x * oth.x + y * oth.y + z * oth.z;
	}

	X_INLINE Vec3<T> cross(const Vec3<T>& oth) const {
		return Vec3<T>(y * oth.z - oth.y * z, z * oth.x - oth.z * x, x * oth.y - oth.x * y);
	}

	T distance(const Vec3<T> &rhs) const
	{
		return static_cast<T>((*this - rhs).length());
	}

	T distanceSquared(const Vec3<T> &rhs) const
	{
		return (*this - rhs).lengthSquared();
	}

	DIST length() const
	{
		return math<DIST>::sqrt((DIST)(x*x + y*y + z*z));
	}

	T lengthSquared() const
	{
		return x*x + y*y + z*z;
	}

	//! Limits the length of a Vec3 to \a maxLength, scaling it proportionally if necessary.
	void limit(T maxLength)
	{
		T lengthSquared = x * x + y * y + z * z;

		if ((lengthSquared > maxLength * maxLength) && (lengthSquared > 0)) {
			T ratio = maxLength / math<T>::sqrt(lengthSquared);
			x *= ratio;
			y *= ratio;
			z *= ratio;
		}
	}

	//! Returns a copy of the Vec3 with its length limited to \a maxLength, scaling it proportionally if necessary.
	Vec3<T> limited(T maxLength) const
	{
		T lengthSquared = x * x + y * y + z * z;

		if ((lengthSquared > maxLength * maxLength) && (lengthSquared > 0)) {
			T ratio = maxLength / math<T>::sqrt(lengthSquared);
			return Vec3<T>(x * ratio, y * ratio, z * ratio);
		}
		else
			return *this;
	}

	Vec3<T>& invert()
	{
		x = -x; y = -y; z = -z;
		return *this;
	}

	Vec3<T> inverse() const
	{
		return Vec3<T>(-x, -y, -z);
	}

	Vec3<T>& normalize()
	{
		T invS = (static_cast<T>(1)) / length();
		x *= invS;
		y *= invS;
		z *= invS;
		return *this;
	}

	Vec3<T> normalized() const
	{
		T invS = (static_cast<T>(1)) / length();
		return Vec3<T>(x * invS, y * invS, z * invS);
	}

	// tests for zero-length
	Vec3<T>& normalizeSafe()
	{
		T s = lengthSquared();
		if (s > 0) {
			T invS = (static_cast<T>(1)) / math<T>::sqrt(s);
			x *= invS;
			y *= invS;
			z *= invS;
		}
		return *this;
	}

	Vec3<T> normalizeSafe() const
	{
		T s = lengthSquared();
		if (s > 0) {
			float invS = (static_cast<T>(1)) / math<T>::sqrt(s);
			return Vec3<T>(x * invS, y * invS, z * invS);
		}
		else
			return *this;
	}

	//! Returns a vector which is orthogonal to \a this
	Vec3<T> getOrthogonal() const
	{
		if (math<T>::abs(y) < static_cast<T>(0.99)) // abs(dot(u, Y)), somewhat arbitrary epsilon
			return Vec3<T>(-z, 0, x); // cross( this, Y )
		else
			return Vec3<T>(0, z, -y); // cross( this, X )
	}

	void rotateX(T angle)
	{
		T sina = math<T>::sin(angle);
		T cosa = math<T>::cos(angle);
		T ry = y * cosa - z * sina;
		T rz = y * sina + z * cosa;
		y = ry;
		z = rz;
	}

	void rotateY(T angle)
	{
		T sina = math<T>::sin(angle);
		T cosa = math<T>::cos(angle);
		T rx = x * cosa - z * sina;
		T rz = x * sina + z * cosa;
		x = rx;
		z = rz;
	}

	void rotateZ(T angle)
	{
		T sina = math<T>::sin(angle);
		T cosa = math<T>::cos(angle);
		T rx = x * cosa - y * sina;
		T ry = x * sina + y * cosa;
		x = rx;
		y = ry;
	}

	void rotate(Vec3<T> axis, T angle)
	{
		T cosa = math<T>::cos(angle);
		T sina = math<T>::sin(angle);

		T rx = (cosa + (1 - cosa) * axis.x * axis.x) * x;
		rx += ((1 - cosa) * axis.x * axis.y - axis.z * sina) * y;
		rx += ((1 - cosa) * axis.x * axis.z + axis.y * sina) * z;

		T ry = ((1 - cosa) * axis.x * axis.y + axis.z * sina) * x;
		ry += (cosa + (1 - cosa) * axis.y * axis.y) * y;
		ry += ((1 - cosa) * axis.y * axis.z - axis.x * sina) * z;

		T rz = ((1 - cosa) * axis.x * axis.z - axis.y * sina) * x;
		rz += ((1 - cosa) * axis.y * axis.z + axis.x * sina) * y;
		rz += (cosa + (1 - cosa) * axis.z * axis.z) * z;

		x = rx;
		y = ry;
		z = rz;
	}

	Vec3<T> lerp(T fact, const Vec3<T> &rhs) const
	{
		return (*this) + (rhs - (*this)) * fact;
	}

	void lerpEq(T fact, const Vec3<T> &rhs)
	{
		x = x + (rhs.x - x) * fact; y = y + (rhs.y - y) * fact; z = z + (rhs.z - z) * fact;
	}

	static Vec3<T> zero()
	{
		return Vec3<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
	}

	static Vec3<T> one()
	{
		return Vec3<T>(static_cast<T>(1), static_cast<T>(1), static_cast<T>(1));
	}

	static Vec3<T> min()
	{
		return Vec3<T>(
			std::numeric_limits<T>::lowest(),
			std::numeric_limits<T>::lowest(),
			std::numeric_limits<T>::lowest()
		);
	}
	static Vec3<T> max()
	{
		return Vec3<T>(
			std::numeric_limits<T>::max(),
			std::numeric_limits<T>::max(),
			std::numeric_limits<T>::max()
		);
	}


	Vec3<T> slerp(T fact, const Vec3<T> &r) const
	{
		T cosAlpha, alpha, sinAlpha;
		T t1, t2;
		Vec3<T> result;

		// get cosine of angle between vectors (-1 -> 1)
		cosAlpha = this->dot(r);

		// get angle (0 -> pi)
		alpha = math<T>::acos(cosAlpha);

		// get sine of angle between vectors (0 -> 1)
		sinAlpha = math<T>::sin(alpha);

		// this breaks down when sinAlpha = 0, i.e. alpha = 0 or pi
		t1 = math<T>::sin((static_cast<T>(1) - fact) * alpha) / sinAlpha;
		t2 = math<T>::sin(fact * alpha) / sinAlpha;

		// interpolate src vectors
		return *this * t1 + r * t2;
	}

	// derived from but not equivalent to Quaternion::squad
	Vec3<T> squad(T t, const Vec3<T> &tangentA, const Vec3<T> &tangentB, const Vec3<T> &end) const
	{
		Vec3<T> r1 = this->slerp(t, end);
		Vec3<T> r2 = tangentA.slerp(t, tangentB);
		return r1.slerp(2 * t * (1 - t), r2);
	}


	// for bounding goats.
	void checkMin(const Vec3<T>& oth) {
		x = math<T>::min(x,oth.x);
		y = math<T>::min(y,oth.y);
		z = math<T>::min(z,oth.z);
	}
	void checkMax(const Vec3<T>& oth) {
		x = math<T>::max(x,oth.x);
		y = math<T>::max(y,oth.y);
		z = math<T>::max(z,oth.z);
	}

	X_INLINE Vec2<T> xy() const { return Vec2<T>(x, y); }


	static Vec3<T> xAxis() { return Vec3<T>(1, 0, 0); }
	static Vec3<T> yAxis() { return Vec3<T>(0, 1, 0); }
	static Vec3<T> zAxis() { return Vec3<T>(0, 0, 1); }

	static Vec3<T> NaN()   { return Vec3<T>(math<T>::NaN(), math<T>::NaN(), math<T>::NaN()); }

	static Vec3<T> abs(const Vec3<T>& vec) {
		return vec.abs();
	}
};



template<typename T>
class Vec4
{
public:
	T x, y, z, w;

	typedef T value_type;
	typedef typename VECTRAIT<T>::DIST	DIST;

	Vec4() : x(0), y(0), z(0), w(0) {}
	Vec4(T X, T Y, T Z, T W = 0) : x(X), y(Y), z(Z), w(W) {}
	Vec4(const Vec4<T>& src) : x(src.x), y(src.y), z(src.z), w(src.w) {}
	explicit Vec4(const T* pSrc) : x(pSrc[0]), y(pSrc[1]), z(pSrc[2]), w(pSrc[3]) {}
	Vec4(const Vec3<T>& src, T W = 0) : x(src.x), y(src.y), z(src.z), w(W) {}

	X_INLINE void set(T X, T Y, T Z, T W) {
		x = X; y = Y; z = Z; w = W;
	}
	X_INLINE void set(const Vec4<T>& src) {
		x = src.x; y = src.y; z = src.z; w = src.w;
	}

	// kinky operators
	X_INLINE int operator!() const { return x == 0 && y == 0 && z == 0 && w == 0; }

	X_INLINE Vec4<T>& operator=(const Vec4<T>& oth)
	{
		x = oth.x;
		y = oth.y;
		z = oth.z;
		w = oth.w;
		return *this;
	}

	template<typename FromT>
	X_INLINE Vec4<T>& operator=(const Vec4<FromT>& oth)
	{
		x = static_cast<T>(oth.x);
		y = static_cast<T>(oth.y);
		z = static_cast<T>(oth.z);
		w = static_cast<T>(oth.w);
		return *this;
	}

	X_INLINE const T& operator[](size_t i) const {
		X_ASSERT(i >= 0 && i < 4, "out of range")(i);
			return (&x)[i];
	}

	X_INLINE T& operator[](size_t i) {
		X_ASSERT(i >= 0 && i < 4, "out of range")(i);
			return (&x)[i];
	}


	X_INLINE const Vec4<T>	operator+(const Vec4<T>& rhs) const { return Vec4<T>(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
	X_INLINE const Vec4<T>	operator-(const Vec4<T>& rhs) const { return Vec4<T>(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
	X_INLINE const Vec4<T>	operator*(const Vec4<T>& rhs) const { return Vec4<T>(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }
	X_INLINE const Vec4<T>	operator/(const Vec4<T>& rhs) const { return Vec4<T>(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w); }

	X_INLINE Vec4<T>&	operator+=(const Vec4<T>& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
	X_INLINE Vec4<T>&	operator-=(const Vec4<T>& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
	X_INLINE Vec4<T>&	operator*=(const Vec4<T>& rhs)	{ x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w; return *this; }
	X_INLINE Vec4<T>&	operator/=(const Vec4<T>& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w; return *this; }
	X_INLINE const Vec4<T>	operator/(T rhs) const { return Vec4<T>(x / rhs, y / rhs, z / rhs, w / rhs); }
	X_INLINE Vec4<T>&	operator+=(T rhs) { x += rhs; y += rhs; z += rhs; w += rhs; return *this; }
	X_INLINE Vec4<T>&	operator-=(T rhs) { x -= rhs; y -= rhs; z -= rhs; w -= rhs; return *this; }
	X_INLINE Vec4<T>&	operator*=(T rhs) { x *= rhs; y *= rhs; z *= rhs; w *= rhs; return *this; }
	X_INLINE Vec4<T>&	operator/=(T rhs) { x /= rhs; y /= rhs; z /= rhs; w /= rhs; return *this; }

	X_INLINE Vec4<T>	operator-() const { return Vec4<T>(-x, -y, -z, -w); }


	X_INLINE bool operator==(const Vec4<T>& oth) const {
		return x == oth.x && y == oth.y && z == oth.z && w == oth.w;
	}

	X_INLINE bool operator!=(const Vec4<T>& oth) const {
		return !(*this == oth);
	}

	X_INLINE Vec4<T> abs(void) const {
		return Vec4<T>(math<T>::abs(x), math<T>::abs(y), math<T>::abs(z), math<T>::abs(w));
	}

	X_INLINE T dot(const Vec4<T>& oth) const {
		return x * oth.x + y * oth.y + z * oth.z + w * oth.w;
	}

	Vec4<T> cross(const Vec4<T> &rhs) const
	{
		return Vec4<T>(y*rhs.z - rhs.y*z, z*rhs.x - rhs.z*x, x*rhs.y - rhs.x*y);
	}

	T distance(const Vec4<T> &rhs) const
	{
		return static_cast<T>((*this - rhs).length());
	}

	T distanceSquared(const Vec4<T> &rhs) const
	{
		return (*this - rhs).lengthSquared();
	}

	DIST length() const
	{
		// For most vector operations, this assumes w to be zero.
		return math<DIST>::sqrt((DIST)(x*x + y*y + z*z + w*w));
	}

	T lengthSquared() const
	{
		// For most vector operations, this assumes w to be zero.
		return x*x + y*y + z*z + w*w;
	}

	Vec4<T>& normalize()
	{
		T invS = (static_cast<T>(1)) / length();
		x *= invS;
		y *= invS;
		z *= invS;
		w *= invS;
		return *this;
	}

	Vec4<T> normalized() const
	{
		T invS = (static_cast<T>(1)) / length();
		return Vec4<T>(x*invS, y*invS, z*invS, w*invS);
	}

	// Tests for zero-length
	Vec4<T>& normalizeSafe()
	{
		T s = lengthSquared();
		if (s > static_cast<T>(0)) {
			T invS = (static_cast<T>(1)) / math<T>::sqrt(s);
			x *= invS;
			y *= invS;
			z *= invS;
			w = static_cast<T>(0);
		}
		return *this;
	}

	//! Limits the length of a Vec4 to \a maxLength, scaling it proportionally if necessary.
	void limit(T maxLength)
	{
		T lenSq = lengthSquared();

		if ((lenSq > maxLength * maxLength) && (lenSq > 0)) {
			T ratio = maxLength / math<T>::sqrt(lenSq);
			x *= ratio;
			y *= ratio;
			z *= ratio;
			w *= ratio;
		}
	}

	//! Returns a copy of the Vec4 with its length limited to \a maxLength, scaling it proportionally if necessary.
	Vec4<T> limited(T maxLength) const
	{
		T lenSq = lengthSquared();

		if ((lenSq > maxLength * maxLength) && (lenSq > 0)) {
			T ratio = maxLength / math<T>::sqrt(lenSq);
			return Vec4<T>(x * ratio, y * ratio, z * ratio, w * ratio);
		}
		else
			return *this;
	}

	Vec4<T>& invert()
	{
		x = -x; y = -y; z = -z; w = -w;
		return *this;
	}

	Vec4<T> inverse() const
	{
		return Vec4<T>(-x, -y, -z, -w);
	}

	Vec4<T> lerp(T fact, const Vec4<T>& r) const
	{
		return (*this) + (r - (*this)) * fact;
	}

	void lerpEq(T fact, const Vec4<T> &rhs)
	{
		x = x + (rhs.x - x) * fact; y = y + (rhs.y - y) * fact; z = z + (rhs.z - z) * fact; w = w + (rhs.w - w) * fact;
	}


	static Vec4<T> zero()
	{
		return Vec4<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
	}

	static Vec4<T> one()
	{
		return Vec4<T>(static_cast<T>(1), static_cast<T>(1), static_cast<T>(1), static_cast<T>(1));
	}

	static Vec4<T> min()
	{
		return Vec4<T>(
			std::numeric_limits<T>::lowest(),
			std::numeric_limits<T>::lowest(),
			std::numeric_limits<T>::lowest(),
			std::numeric_limits<T>::lowest()
			);
	}

	static Vec4<T> max()
	{
		return Vec4<T>(
			std::numeric_limits<T>::max(),
			std::numeric_limits<T>::max(),
			std::numeric_limits<T>::max(),
			std::numeric_limits<T>::max()
		);
	}


	Vec4<T> slerp(T fact, const Vec3<T> &r) const
	{
		T cosAlpha, alpha, sinAlpha;
		T t1, t2;
		Vec4<T> result;

		// get cosine of angle between vectors (-1 -> 1)
		cosAlpha = this->dot(r);

		// get angle (0 -> pi)
		alpha = math<T>::acos(cosAlpha);

		// get sine of angle between vectors (0 -> 1)
		sinAlpha = math<T>::sin(alpha);

		// this breaks down when sinAlpha = 0, i.e. alpha = 0 or pi
		t1 = math<T>::sin((static_cast<T>(1) - fact) * alpha) / sinAlpha;
		t2 = math<T>::sin(fact * alpha) / sinAlpha;

		// interpolate src vectors
		return *this * t1 + r * t2;
	}

	// derived from but not equivalent to Quaternion::squad
	Vec4<T> squad(T t, const Vec4<T> &tangentA, const Vec4<T> &tangentB, const Vec4<T> &end) const
	{
		Vec4<T> r1 = this->slerp(t, end);
		Vec4<T> r2 = tangentA.slerp(t, tangentB);
		return r1.slerp(2 * t * (1 - t), r2);
	}

	X_INLINE Vec3<T> xyz() const { return Vec3<T>(x,y,z); }
	X_INLINE Vec2<T> xy() const { return Vec2<T>(x, y); }

	static Vec4<T> xAxis() { return Vec4<T>(1, 0, 0, 0); }
	static Vec4<T> yAxis() { return Vec4<T>(0, 1, 0, 0); }
	static Vec4<T> zAxis() { return Vec4<T>(0, 0, 1, 0); }
	static Vec4<T> wAxis() { return Vec4<T>(0, 0, 0, 1); }

	static Vec4<T> NaN()   { return Vec4<T>(math<T>::NaN(), math<T>::NaN(), math<T>::NaN(), math<T>::NaN()); }

	static Vec4<T> abs(const Vec4<T>& vec) {
		return vec.abs();
	}
};



template<typename T>
class Vec5
{
public:
	T x, y, z, s, t;

	typedef T value_type;
	typedef typename VECTRAIT<T>::DIST	DIST;

	Vec5() : x(0), y(0), z(0), s(0), t(0) {}
	Vec5(T X, T Y, T Z, T S = 0, T t = 0) : x(X), y(Y), z(Z), s(S), t(t) {}
	Vec5(const Vec5<T>& src) : x(src.x), y(src.y), z(src.z), s(src.s), t(src.t) {}
	explicit Vec5(const Vec3<T>& src) : x(src.x), y(src.y), z(src.z), s(0), t(0) {}
	explicit Vec5(const T* pSrc) : x(pSrc[0]), y(pSrc[1]), z(pSrc[2]), s(pSrc[3]), t(pSrc[4]) {}

	X_INLINE void set(T X, T Y, T Z, T S, T _T) {
		x = X; y = Y; z = Z; s = S; t = _T;
	}
	X_INLINE void set(const Vec5<T>& src) {
		x = src.x; y = src.y; z = src.z; s = src.s; t = src.t;
	}

	X_INLINE Vec3<T>& asVec3(void) {
		return *reinterpret_cast<Vec3<T> *>(this);
	}
	X_INLINE const Vec3<T>& asVec3(void) const {
		return *reinterpret_cast<const Vec3<T> *>(this);
	}


	// kinky operators
	X_INLINE int operator!() const { return x == 0 && y == 0 && z == 0 && s == 0 && t == 0; }

	X_INLINE Vec5<T>& operator=(const Vec5<T>& oth)
	{
		x = oth.x;
		y = oth.y;
		z = oth.z;
		s = oth.s;
		t = oth.t;
		return *this;
	}

	template<typename FromT>
	X_INLINE Vec5<T>& operator=(const Vec5<FromT>& oth)
	{
		x = static_cast<T>(oth.x);
		y = static_cast<T>(oth.y);
		z = static_cast<T>(oth.z);
		s = static_cast<T>(oth.s);
		t = static_cast<T>(oth.t);
		return *this;
	}

	X_INLINE Vec5<T>& operator=(const Vec3<T>& oth)
	{
		x = oth.x;
		y = oth.y;
		z = oth.z;
		s = t = 0;
		return *this;
	}

	X_INLINE const T& operator[](size_t i) const {
		X_ASSERT(i >= 0 && i < 5, "out of range")(i);
		return (&x)[i];
	}

	X_INLINE T& operator[](size_t i) {
		X_ASSERT(i >= 0 && i < 5, "out of range")(i);
		return (&x)[i];
	}

	X_INLINE const Vec5<T>	operator+(const Vec5<T>& rhs) const { return Vec5<T>(x + rhs.x, y + rhs.y, z + rhs.z, s + rhs.s, t + rhs.t); }
	X_INLINE const Vec5<T>	operator-(const Vec5<T>& rhs) const { return Vec5<T>(x - rhs.x, y - rhs.y, z - rhs.z, s - rhs.s, t - rhs.t); }
	X_INLINE const Vec5<T>	operator*(const Vec5<T>& rhs) const { return Vec5<T>(x * rhs.x, y * rhs.y, z * rhs.z, s * rhs.s, t * rhs.t); }
	X_INLINE const Vec5<T>	operator/(const Vec5<T>& rhs) const { return Vec5<T>(x / rhs.x, y / rhs.y, z / rhs.z, s / rhs.s, t / rhs.t); }

	X_INLINE Vec5<T>&	operator+=(const Vec5<T>& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; s += rhs.s; t += rhs.t; return *this; }
	X_INLINE Vec5<T>&	operator-=(const Vec5<T>& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; s -= rhs.s; t -= rhs.t; return *this; }
	X_INLINE Vec5<T>&	operator*=(const Vec5<T>& rhs)	{ x *= rhs.x; y *= rhs.y; z *= rhs.z; s *= rhs.s; t *= rhs.t; return *this; }
	X_INLINE Vec5<T>&	operator/=(const Vec5<T>& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; s /= rhs.s; t /= rhs.t; return *this; }
	X_INLINE const Vec5<T>	operator/(T rhs) const { return Vec5<T>(x / rhs, y / rhs, z / rhs.z, s / rhs.s, t / rhs.t); }
	X_INLINE Vec5<T>&	operator+=(T rhs) { x += rhs; y += rhs; z += rhs; s += rhs; t += rhs.t; return *this; }
	X_INLINE Vec5<T>&	operator-=(T rhs) { x -= rhs; y -= rhs; z -= rhs; s -= rhs; t -= rhs.t; return *this; }
	X_INLINE Vec5<T>&	operator*=(T rhs) { x *= rhs; y *= rhs; z *= rhs; s *= rhs; t *= rhs.t; return *this; }
	X_INLINE Vec5<T>&	operator/=(T rhs) { x /= rhs; y /= rhs; z /= rhs; s /= rhs; t /= rhs.t; return *this; }

	X_INLINE Vec5<T>	operator-() const { return Vec5<T>(-x, -y, -z, -s, -t); }


	X_INLINE bool operator==(const Vec5<T>& oth) const {
		return x == oth.x && y == oth.y && z == oth.z && s == oth.s && t == oth.t;
	}

	X_INLINE bool operator!=(const Vec5<T>& oth) const {
		return !(*this == oth);
	}

	X_INLINE Vec5<T> abs(void) const {
		return Vec5<T>(math<T>::abs(x), math<T>::abs(y), math<T>::abs(z), math<T>::abs(s), math<T>::abs(t));
	}

	X_INLINE T dot(const Vec5<T>& oth) const {
		return x * oth.x + y * oth.y + z * oth.z;
	}

	Vec5<T> cross(const Vec5<T> &rhs) const
	{
		return Vec5<T>(y*rhs.z - rhs.y*z, z*rhs.x - rhs.z*x, x*rhs.y - rhs.x*y);
	}

	T distance(const Vec5<T> &rhs) const
	{
		return static_cast<T>((*this - rhs).length());
	}

	T distanceSquared(const Vec5<T> &rhs) const
	{
		return (*this - rhs).lengthSquared();
	}

	DIST length() const
	{
		// For most vector operations, this assumes s & t to be zero.
		return math<DIST>::sqrt((DIST)(x*x + y*y + z*z + s*s + t*t));
	}

	T lengthSquared() const
	{
		// For most vector operations, this assumes s & t to be zero.
		return x*x + y*y + z*z + s*s + t*t;
	}

	Vec5<T>& normalize()
	{
		T invS = (static_cast<T>(1)) / length();
		x *= invS;
		y *= invS;
		z *= invS;
		s *= invS;
		t *= invS;
		return *this;
	}

	Vec5<T> normalized() const
	{
		T invS = (static_cast<T>(1)) / length();
		return Vec5<T>(x*invS, y*invS, z*invS, s*invS, t*invS);
	}

	// Tests for zero-length
	Vec5<T>& normalizeSafe()
	{
		T s = lengthSquared();
		if (s > 0) {
			T invS = (static_cast<T>(1)) / math<T>::sqrt(s);
			x *= invS;
			y *= invS;
			z *= invS;
			s = static_cast<T>(0);
			t = static_cast<T>(0);
		}
		return *this;
	}



	static Vec5<T> zero()
	{
		return Vec5<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
	}

	static Vec5<T> one()
	{
		return Vec5<T>(static_cast<T>(1), static_cast<T>(1), static_cast<T>(1), static_cast<T>(1), static_cast<T>(1));
	}

	static Vec5<T> min()
	{
		return Vec5<T>(
			std::numeric_limits<T>::lowest(),
			std::numeric_limits<T>::lowest(),
			std::numeric_limits<T>::lowest(),
			std::numeric_limits<T>::lowest(),
			std::numeric_limits<T>::lowest()
		);
	}
	static Vec5<T> max()
	{
		return Vec5<T>(
			std::numeric_limits<T>::max(),
			std::numeric_limits<T>::max(),
			std::numeric_limits<T>::max(),
			std::numeric_limits<T>::max(),
			std::numeric_limits<T>::max()
		);
	}

	X_INLINE Vec3<T> xyz() const { return Vec3<T>(x, y, z); }
	X_INLINE Vec2<T> xy() const { return Vec2<T>(x, y); }

	static Vec5<T> NaN() {
		return Vec5<T>(math<T>::NaN(), math<T>::NaN(), math<T>::NaN(), math<T>::NaN(), math<T>::NaN());
	}

	static Vec5<T> abs(const Vec5<T>& vec) {
		return vec.abs();
	}
};


// make it possible for you to do.
// Vec2 vec;
// Vec2 goat = (vec * 2);
template<typename T, typename Y> inline Vec2<T> operator *(Y s, const Vec2<T> &v) { return Vec2<T>(v.x * s, v.y * s); }
template<typename T, typename Y> inline Vec2<T> operator *(const Vec2<T> &v, Y s) { return Vec2<T>(v.x * s, v.y * s); }
template<typename T, typename Y> inline Vec3<T> operator *(Y s, const Vec3<T> &v) { return Vec3<T>(v.x * s, v.y * s, v.z * s); }
template<typename T, typename Y> inline Vec3<T> operator *(const Vec3<T> &v, Y s) { return Vec3<T>(v.x * s, v.y * s, v.z * s); }
template<typename T, typename Y> inline Vec4<T> operator *(Y s, const Vec4<T> &v) { return Vec4<T>(v.x * s, v.y * s, v.z * s, v.w * s); }
template<typename T, typename Y> inline Vec4<T> operator *(const Vec4<T> &v, Y s) { return Vec4<T>(v.x * s, v.y * s, v.z * s, v.w * s); }
template<typename T, typename Y> inline Vec5<T> operator *(Y s, const Vec5<T> &v) { return Vec5<T>(v.x * s, v.y * s, v.z * s, v.s * s, v.t * s); }
template<typename T, typename Y> inline Vec5<T> operator *(const Vec5<T> &v, Y s) { return Vec5<T>(v.x * s, v.y * s, v.z * s, v.s * s, v.t * s); }



template <typename T> T dot(const Vec2<T>& a, const Vec2<T>& b) { return a.dot(b); }
template <typename T> T dot(const Vec3<T>& a, const Vec3<T>& b) { return a.dot(b); }
template <typename T> T dot(const Vec4<T>& a, const Vec4<T>& b) { return a.dot(b); }

template <typename T> Vec3<T> cross(const Vec3<T>& a, const Vec3<T>& b) { return a.cross(b); }
template <typename T> Vec4<T> cross(const Vec4<T>& a, const Vec4<T>& b) { return a.cross(b); }


template <typename T> bool operator<(const Vec3<T>& a, const Vec3<T>& b) {
	return a.x < b.x && a.y < b.y && a.z < b.z;
}
template <typename T> bool operator<=(const Vec3<T>& a, const Vec3<T>& b) {
	return a.x <= b.x && a.y <= b.y && a.z <= b.z;
}

template <typename T> bool operator>(const Vec3<T>& a, const Vec3<T>& b) {
	return a.x > b.x && a.y > b.y && a.z > b.z;
}
template <typename T> bool operator>=(const Vec3<T>& a, const Vec3<T>& b) {
	return a.x >= b.x && a.y >= b.y && a.z >= b.z;
}

template<typename T>
X_INLINE T operator | (const Vec3<T> &v0, const Vec3<T> &v1)
{
	return v0.dot(v1);
}


typedef Vec2<int32_t>	Vec2i;
typedef Vec2<float32_t>	Vec2f;
typedef Vec2<float64_t>	Vec2d;
typedef Vec3<int32_t>	Vec3i;
typedef Vec3<float32_t>	Vec3f;
typedef Vec3<float64_t>	Vec3d;
typedef Vec4<int32_t>	Vec4i;
typedef Vec4<float32_t>	Vec4f;
typedef Vec4<float64_t>	Vec4d;
typedef Vec5<int32_t>	Vec5i;
typedef Vec5<float32_t>	Vec5f;
typedef Vec5<float64_t>	Vec5d;


#endif // !_X_MATH_VEC_H_
