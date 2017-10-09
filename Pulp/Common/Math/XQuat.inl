
// get axis-angle representation's axis
template<typename T>
X_INLINE Vec3<T> Quat<T>::getAxis() const
{
	T cos_angle = w;
	T invLen = static_cast<T>(1.0) / math<T>::sqrt(static_cast<T>(1.0) - cos_angle * cos_angle);

	return v * invLen;
}

// get axis-angle representation's angle in radians
template<typename T>
X_INLINE T Quat<T>::getAngle() const
{
	T cos_angle = w;
	return math<T>::acos(cos_angle) * 2;
}

template<typename T>
X_INLINE T Quat<T>::getPitch() const
{
	return math<T>::atan2((T)2 * (v.y * v.z + w * v.x), w * w - v.x * v.x - v.y * v.y + v.z * v.z);
}

template<typename T>
X_INLINE T Quat<T>::getYaw() const
{
	return math<T>::asin((T)-2 * (v.x * v.z - w * v.y));
}

template<typename T>
X_INLINE T Quat<T>::getRoll() const
{
	return math<T>::atan2((T)2 * (v.x * v.y + w * v.z), w * w + v.x * v.x - v.y * v.y - v.z * v.z);
}

template<typename T>
X_INLINE T Quat<T>::dot(const Quat<T> &quat) const
{
	return w * quat.w + v.dot(quat.v);
}

template<typename T>
X_INLINE T Quat<T>::length() const
{
	return (T)std::sqrt(w*w + v.lengthSquared());
}

template<typename T>
X_INLINE T Quat<T>::lengthSquared() const
{
	return w * w + v.lengthSquared();
}

template<typename T>
X_INLINE Vec3<T> Quat<T>::getEuler(void) const
{
	Vec3<T> euler;

	euler.x = getPitch();
	euler.y = getYaw();
	euler.z = getRoll();
	return euler;
}

template<typename T>
X_INLINE Vec3<T> Quat<T>::getEulerDegrees(void) const
{
	Vec3<T> euler;

	euler.x = toDegrees(getPitch());
	euler.y = toDegrees(getYaw());
	euler.z = toDegrees(getRoll());
	return euler;
}




template<typename T>
X_INLINE Quat<T>& Quat<T>::normalize()
{
	if (T len = length()) {
		w /= len;
		v /= len;
	}
	else {
		w = static_cast<T>(1.0);
		v = Vec3<T>(0, 0, 0);
	}
	return *this;
}

template<typename T>
X_INLINE Quat<T> Quat<T>::normalized() const
{
	Quat<T> result = *this;

	if (T len = length()) {
		result.w /= len;
		result.v /= len;
	}
	else {
		result.w = static_cast<T>(1.0);
		result.v = Vec3<T>(0, 0, 0);
	}

	return result;
}


// For unit Quat, from Advanced Animation and 
// Rendering Techniques by Watt and Watt, Page 366:
template<typename T>
X_INLINE Quat<T> Quat<T>::log() const
{
	T theta = math<T>::acos(core::Min(w, (T) 1.0));

	if (theta == 0)
		return Quat<T>(v, 0);

	T sintheta = math<T>::sin(theta);

	T k;
	if (abs(sintheta) < 1 && abs(theta) >= 3.402823466e+38F * abs(sintheta))
		k = 1;
	else
		k = theta / sintheta;

	return Quat<T>((T)0, v.x * k, v.y * k, v.z * k);
}

// For pure Quat (zero scalar part):
// from Advanced Animation and Rendering
// Techniques by Watt and Watt, Page 366:
template<typename T>
X_INLINE Quat<T> Quat<T>::exp() const
{
	T theta = v.length();
	T sintheta = math<T>::sin(theta);

	T k;
	if (abs(theta) < 1 && abs(sintheta) >= 3.402823466e+38F * abs(theta))
		k = 1;
	else
		k = sintheta / theta;

	T costheta = math<T>::cos(theta);

	return Quat<T>(costheta, v.x * k, v.y * k, v.z * k);
}


template<typename T>
X_INLINE Quat<T> Quat<T>::inverse() const
{
	T norm = w * w + v.x * v.x + v.y * v.y + v.z * v.z;
	T normRecip = static_cast<T>(1.0f) / norm;
	return Quat<T>(normRecip * w, -normRecip * v.x, -normRecip * v.y, -normRecip * v.z);
}

template<typename T>
X_INLINE Quat<T> Quat<T>::inverted() const
{
	return Quat(w, -v);
}

template<typename T>
X_INLINE Quat<T>& Quat<T>::invert()
{
	*this = Quat(w, -v);
	return *this;
}

template<typename T>
X_INLINE Quat<T> Quat<T>::diff(const Quat<T>& oth) const
{
	Quat<T> in = inverse();
	return in * oth;
}

template<typename T>
X_INLINE void Quat<T>::set(T aW, T x, T y, T z)
{
	w = aW;
	v = Vec3<T>(x, y, z);
}

template<typename T>
X_INLINE void Quat<T>::set(const Vec3<T> &from, const Vec3<T> &to)
{
	Vec3<T> axis = from.cross(to);

	set(from.dot(to), axis.x, axis.y, axis.z);
	normalize();

	w += static_cast<T>(1.0);

	if (w <= EPSILON) {
		if (from.z * from.z > from.x * from.x) {
			set(static_cast<T>(0.0), static_cast<T>(0.0), from.z, -from.y);
		}
		else {
			set(static_cast<T>(0.0), from.y, -from.x, static_cast<T>(0.0));
		}
	}

	normalize();
}

template<typename T>
X_INLINE void Quat<T>::set(const Vec3<T> &axis, T radians)
{
	w = math<T>::cos(radians / 2);
	v = axis.normalized() * math<T>::sin(radians / 2);
}

// assumes ZYX rotation order and radians
template<typename T>
X_INLINE void Quat<T>::set(T pitch, T yaw, T roll)
{
#if 1
	pitch *= T(0.5);
	yaw *= T(0.5);
	roll *= T(0.5);

	const T sinPitch(math<T>::sin(pitch));
	const T cosPitch(math<T>::cos(pitch));
	const T sinYaw(math<T>::sin(yaw));
	const T cosYaw(math<T>::cos(yaw));
	const T sinRoll(math<T>::sin(roll));
	const T cosRoll(math<T>::cos(roll));
	const T cosPitchCosYaw(cosPitch*cosYaw);
	const T sinPitchSinYaw(sinPitch*sinYaw);

	v.x = sinRoll * cosPitchCosYaw - cosRoll * sinPitchSinYaw;
	v.y = cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw;
	v.z = cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw;
	w = cosRoll * cosPitchCosYaw + sinRoll * sinPitchSinYaw;

#else
#if 0
	T sinPitch, cosPitch;
	T sinYaw, cosYaw;
	T sinRoll, cosRoll;

	math<T>::sincos(::toRadians(pitch) * T(0.5f), sinPitch, cosPitch);
	math<T>::sincos(::toRadians(yaw)   * T(0.5f), sinYaw, cosYaw);
	math<T>::sincos(::toRadians(roll)  * T(0.5f), sinRoll, cosRoll);

	const T cosPitchCosYaw(cosPitch*cosYaw);
	const T sinPitchSinYaw(sinPitch*sinYaw);

	v.x = sinRoll * cosPitchCosYaw - cosRoll * sinPitchSinYaw;
	v.y = cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw;
	v.z = cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw;
	w = cosRoll * cosPitchCosYaw + sinRoll * sinPitchSinYaw;
#else
	T Cx, Sx, Cy, Sy, Cz, Sz;
	T sxcy, cxcy, sxsy, cxsy;

	math<T>::sincos(::toRadians(pitch) * T(0.5f), Cx, Sx);
	math<T>::sincos(::toRadians(yaw)   * T(0.5f), Cy, Sy);
	math<T>::sincos(::toRadians(roll)  * T(0.5f), Cz, Sz);

	// multiply it out
	sxcy = Sx * Cy;
	cxcy = Cx * Cy;
	sxsy = Sx * Sy;
	cxsy = Cx * Sy;

	v.x = Sx * Cy * Cz + Cx * Sy * Sz;
	v.y = Cx * Sy * Cz - Sx * Cy * Sz;
	v.z = Cx * Cy * Sz + Sx * Sy * Cx;
	w = Cx * Cy * Cz - Sx * Sy * Sz;
#endif
#endif
}


template<typename T>
X_INLINE void Quat<T>::getAxisAngle(Vec3<T> *axis, T *radians) const
{
	T cos_angle = w;
	*radians = math<T>::acos(cos_angle) * 2;
	T invLen = static_cast<T>(1.0) / math<T>::sqrt(static_cast<T>(1.0) - cos_angle * cos_angle);

	axis->x = v.x * invLen;
	axis->y = v.y * invLen;
	axis->z = v.z * invLen;
}

template<typename T>
X_INLINE Matrix33<T> Quat<T>::toMatrix33() const
{
	Matrix33<T> mV;
	T xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

	xs = v.x + v.x;
	ys = v.y + v.y;
	zs = v.z + v.z;
	wx = w * xs;
	wy = w * ys;
	wz = w * zs;
	xx = v.x * xs;
	xy = v.x * ys;
	xz = v.x * zs;
	yy = v.y * ys;
	yz = v.y * zs;
	zz = v.z * zs;

	mV[0] = T(1) - (yy + zz);
	mV[3] = xy - wz;
	mV[6] = xz + wy;

	mV[1] = xy + wz;
	mV[4] = T(1) - (xx + zz);
	mV[7] = yz - wx;

	mV[2] = xz - wy;
	mV[5] = yz + wx;
	mV[8] = T(1) - (xx + yy);

	return mV;
}

template<typename T>
X_INLINE Matrix44<T> Quat<T>::toMatrix44() const
{
	Matrix44<T> mV;
	T xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

	xs = v.x + v.x;
	ys = v.y + v.y;
	zs = v.z + v.z;
	wx = w * xs;
	wy = w * ys;
	wz = w * zs;
	xx = v.x * xs;
	xy = v.x * ys;
	xz = v.x * zs;
	yy = v.y * ys;
	yz = v.y * zs;
	zz = v.z * zs;

	mV[0] = T(1) - (yy + zz);
	mV[4] = xy - wz;
	mV[8] = xz + wy;
	mV[12] = 0;

	mV[1] = xy + wz;
	mV[5] = T(1) - (xx + zz);
	mV[9] = yz - wx;
	mV[13] = 0;

	mV[2] = xz - wy;
	mV[6] = yz + wx;
	mV[10] = T(1) - (xx + yy);
	mV[14] = 0;

	mV[3] = 0;
	mV[7] = 0;
	mV[11] = 0;
	mV[15] = T(1);

	return mV;
}

template<typename T>
X_INLINE Quat<T> Quat<T>::lerp(T t, const Quat<T> &end) const
{
	// get cos of "angle" between Quats
	float cosTheta = dot(end);

	// initialize result
	Quat<T> result = end * t;

	// if "angle" between Quats is less than 90 degrees
	if (cosTheta >= EPSILON) {
		// use standard interpolation
		result += *this * (static_cast<T>(1.0) - t);
	}
	else {
		// otherwise, take the shorter path
		result += *this * (t - static_cast<T>(1.0));
	}

	return result;
}

// This method does *not* interpolate along the shortest
// arc between q1 and q2.  If you desire interpolation
// along the shortest arc, and q1.dot( q2 ) is negative, then
// consider flipping the second Quat explicitly.
//
// NOTE: the difference between this and slerp isn't clear, but we're using
// the Don Hatch / ilmbase squad code which explicity requires this impl. of slerp
// so I'm leaving it for now
template<typename T>
X_INLINE Quat<T> Quat<T>::slerpShortestUnenforced(T t, const Quat<T> &end) const
{
	Quat<T> d = *this - end;
	T lengthD = math<T>::sqrt(this->dot(end));

	Quat<T> st = *this + end;
	T lengthS = math<T>::sqrt(st.dot(st));

	T a = 2 * math<T>::atan2(lengthD, lengthS);;
	T s = 1 - t;

	Quat<T> q(*this * (sinx_over_x(s * a) / sinx_over_x(a) * s) +
		end * (sinx_over_x(t * a) / sinx_over_x(a) * t));

	return q.normalized();
}

template<typename T>
X_INLINE Quat<T> Quat<T>::slerp(T t, const Quat<T> &end) const
{
	// get cosine of "angle" between Quats
	T cosTheta = this->dot(end);
	T startInterp, endInterp;

	// if "angle" between Quats is less than 90 degrees
	if (cosTheta >= EPSILON) {
		// if angle is greater than zero
		if ((static_cast<T>(1.0) - cosTheta) > EPSILON) {
			// use standard slerp
			T theta = math<T>::acos(cosTheta);
			T recipSinTheta = static_cast<T>(1.0) / math<T>::sin(theta);

			startInterp = math<T>::sin((static_cast<T>(1.0) - t) * theta) * recipSinTheta;
			endInterp = math<T>::sin(t * theta) * recipSinTheta;
		}
		// angle is close to zero
		else {
			// use linear interpolation
			startInterp = static_cast<T>(1.0) - t;
			endInterp = t;
		}
	}
	// otherwise, take the shorter route
	else {
		// if angle is less than 180 degrees
		if ((static_cast<T>(1.0) + cosTheta) > EPSILON) {
			// use slerp w/negation of start Quat
			T theta = math<T>::acos(-cosTheta);
			T recipSinTheta = static_cast<T>(1.0) / math<T>::sin(theta);

			startInterp = math<T>::sin((t - static_cast<T>(1.0)) * theta) * recipSinTheta;
			endInterp = math<T>::sin(t * theta) * recipSinTheta;
		}
		// angle is close to 180 degrees
		else {
			// use lerp w/negation of start Quat
			startInterp = t - static_cast<T>(1.0);
			endInterp = t;
		}
	}

	return *this * startInterp + end * endInterp;
}

// Spherical Quadrangle Interpolation -
// from Advanced Animation and Rendering
// Techniques by Watt and Watt, Page 366:
// It constructs a spherical cubic interpolation as 
// a series of three spherical linear interpolations 
// of a quadrangle of unit Quats. 
template<typename T>
X_INLINE Quat<T> Quat<T>::squadShortestEnforced(T t, const Quat<T> &qa, const Quat<T> &qb, const Quat<T> &q2) const
{
	Quat<T> r1;
	if (this->dot(q2) >= 0)
		r1 = this->slerpShortestUnenforced(t, q2);
	else
		r1 = this->slerpShortestUnenforced(t, q2.inverted());

	Quat<T> r2;
	if (qa.dot(qb) >= 0)
		r2 = qa.slerpShortestUnenforced(t, qb);
	else
		r2 = qa.slerpShortestUnenforced(t, qb.inverted());

	if (r1.dot(r2) >= 0)
		return r1.slerpShortestUnenforced(2 * t * (1 - t), r2);
	else
		return r1.slerpShortestUnenforced(2 * t * (1 - t), r2.inverted());
}

template<typename T>
X_INLINE Quat<T> Quat<T>::squad(T t, const Quat<T> &qa, const Quat<T> &qb, const Quat<T> &q2) const
{
	Quat<T> r1 = this->slerp(t, q2);
	Quat<T> r2 = qa.slerp(t, qb);
	return r1.slerp(2 * t * (1 - t), r2);
}

// Spherical Cubic Spline Interpolation -
// from Advanced Animation and Rendering
// Techniques by Watt and Watt, Page 366:
// A spherical curve is constructed using three
// spherical linear interpolations of a quadrangle
// of unit Quats: q1, qa, qb, q2.
// Given a set of Quat keys: q0, q1, q2, q3,
// this routine does the interpolation between
// q1 and q2 by constructing two intermediate
// Quats: qa and qb. The qa and qb are 
// computed by the intermediate function to 
// guarantee the continuity of tangents across
// adjacent cubic segments. The qa represents in-tangent
// for q1 and the qb represents the out-tangent for q2.
// 
// The q1 q2 is the cubic segment being interpolated. 
// The q0 is from the previous adjacent segment and q3 is 
// from the next adjacent segment. The q0 and q3 are used
// in computing qa and qb.
template<typename T>
X_INLINE Quat<T> Quat<T>::spline(T t, const Quat<T> &q1,
	const Quat<T> &q2, const Quat<T> &q3) const
{
	Quat<T> qa = splineIntermediate(*this, q1, q2);
	Quat<T> qb = splineIntermediate(q1, q2, q3);
	Quat<T> result = q1.squadShortestEnforced(t, qa, qb, q2);

	return result;
}

template<typename T>
X_INLINE void Quat<T>::set(const Matrix33<T> &m)
{
	//T trace = m.m[0] + m.m[4] + m.m[8];
	const T trace = m.trace();
	if (trace > (T)0.0)
	{
		const T s = math<T>::sqrt(trace + (T)1.0);
		const T recip = (T)0.5 / s;
		w = s * (T)0.5;
		v.x = (m.at(2, 1) - m.at(1, 2)) * recip;
		v.y = (m.at(0, 2) - m.at(2, 0)) * recip;
		v.z = (m.at(1, 0) - m.at(0, 1)) * recip;
	}
	else
	{
		uint32_t i = 0;
		if (m.at(1, 1) > m.at(0, 0)) {
			i = 1;
		}
		if (m.at(2, 2) > m.at(i, i)) {
			i = 2;
		}

		uint32_t j = (i + 1) % 3;
		uint32_t k = (j + 1) % 3;
		const T s = math<T>::sqrt(m.at(i, i) - m.at(j, j) - m.at(k, k) + (T)1.0);
		const T recip = (T)0.5 / s;
		w = (m.at(k, j) - m.at(j, k)) * recip;
		(*this)[i] = (T)0.5 * s;
		(*this)[j] = (m.at(j, i) + m.at(i, j)) * recip;
		(*this)[k] = (m.at(k, i) + m.at(i, k)) * recip;
	}
}

template<typename T>
X_INLINE void Quat<T>::set(const Matrix34<T> &m)
{
	/// same as above
	T trace = m.trace();
	if (trace > (T)0.0)
	{
		T s = math<T>::sqrt(trace + (T)1.0);
		w = s * (T)0.5;
		T recip = (T)0.5 / s;
		v.x = (m.at(2, 1) - m.at(1, 2)) * recip;
		v.y = (m.at(0, 2) - m.at(2, 0)) * recip;
		v.z = (m.at(1, 0) - m.at(0, 1)) * recip;
	}
	else
	{
		unsigned int i = 0;
		if (m.at(1, 1) > m.at(0, 0))
			i = 1;
		if (m.at(2, 2) > m.at(i, i))
			i = 2;
		unsigned int j = (i + 1) % 3;
		unsigned int k = (j + 1) % 3;
		T s = math<T>::sqrt(m.at(i, i) - m.at(j, j) - m.at(k, k) + (T)1.0);
		(*this)[i] = (T)0.5 * s;
		T recip = (T)0.5 / s;
		w = (m.at(k, j) - m.at(j, k)) * recip;
		(*this)[j] = (m.at(j, i) + m.at(i, j)) * recip;
		(*this)[k] = (m.at(k, i) + m.at(i, k)) * recip;
	}
}

template<typename T>
X_INLINE void Quat<T>::set(const Matrix44<T> &m)
{
	T trace = m.trace();
	if (trace > (T)0.0)
	{
		T s = math<T>::sqrt(trace + (T)1.0);
		w = s * (T)0.5;
		T recip = (T)0.5 / s;
		v.x = (m.at(2, 1) - m.at(1, 2)) * recip;
		v.y = (m.at(0, 2) - m.at(2, 0)) * recip;
		v.z = (m.at(1, 0) - m.at(0, 1)) * recip;
	}
	else
	{
		unsigned int i = 0;
		if (m.at(1, 1) > m.at(0, 0))
			i = 1;
		if (m.at(2, 2) > m.at(i, i))
			i = 2;
		unsigned int j = (i + 1) % 3;
		unsigned int k = (j + 1) % 3;
		T s = math<T>::sqrt(m.at(i, i) - m.at(j, j) - m.at(k, k) + (T)1.0);
		(*this)[i] = (T)0.5 * s;
		T recip = (T)0.5 / s;
		w = (m.at(k, j) - m.at(j, k)) * recip;
		(*this)[j] = (m.at(j, i) + m.at(i, j)) * recip;
		(*this)[k] = (m.at(k, i) + m.at(i, k)) * recip;
	}
}

// Operators
template<typename T>
X_INLINE Quat<T>& Quat<T>::operator=(const Quat<T> &rhs)
{
	v = rhs.v;
	w = rhs.w;
	return *this;
}

template<typename T>
template<typename FromT>
X_INLINE Quat<T>& Quat<T>::operator=(const Quat<FromT> &rhs)
{
	v = rhs.v;
	w = static_cast<T>(rhs.w);
	return *this;
}

template<typename T>
X_INLINE const Quat<T> Quat<T>::operator+(const Quat<T> &rhs) const
{
	const Quat<T>& lhs = *this;
	return Quat<T>(lhs.w + rhs.w, lhs.v.x + rhs.v.x, lhs.v.y + rhs.v.y, lhs.v.z + rhs.v.z);
}

// post-multiply operator, similar to matrices, but different from Shoemake
// Concatenates 'rhs' onto 'this'
template<typename T>
X_INLINE const Quat<T> Quat<T>::operator*(const Quat<T> &rhs) const
{
	return Quat<T>(rhs.w*w - rhs.v.x*v.x - rhs.v.y*v.y - rhs.v.z*v.z,
		rhs.w*v.x + rhs.v.x*w + rhs.v.y*v.z - rhs.v.z*v.y,
		rhs.w*v.y + rhs.v.y*w + rhs.v.z*v.x - rhs.v.x*v.z,
		rhs.w*v.z + rhs.v.z*w + rhs.v.x*v.y - rhs.v.y*v.x);
}

template<typename T>
X_INLINE const Quat<T> Quat<T>::operator*(T rhs) const
{
	return Quat<T>(w * rhs, v.x * rhs, v.y * rhs, v.z * rhs);
}

// transform a vector by the Quat
template<typename T>
X_INLINE const Vec3<T> Quat<T>::operator*(const Vec3<T> &vec) const
{
	T vMult = T(2) * (v.x * vec.x + v.y * vec.y + v.z * vec.z);
	T crossMult = T(2) * w;
	T pMult = crossMult * w - T(1);

	return Vec3<T>(pMult * vec.x + vMult * v.x + crossMult * (v.y * vec.z - v.z * vec.y),
		pMult * vec.y + vMult * v.y + crossMult * (v.z * vec.x - v.x * vec.z),
		pMult * vec.z + vMult * v.z + crossMult * (v.x * vec.y - v.y * vec.x));
}

template<typename T>
X_INLINE const Quat<T> Quat<T>::operator-(const Quat<T> &rhs) const
{
	const Quat<T>& lhs = *this;
	return Quat<T>(lhs.w - rhs.w, lhs.v.x - rhs.v.x, lhs.v.y - rhs.v.y, lhs.v.z - rhs.v.z);
}

template<typename T>
X_INLINE Quat<T>& Quat<T>::operator+=(const Quat<T> &rhs)
{
	w += rhs.w;
	v += rhs.v;
	return *this;
}

template<typename T>
X_INLINE Quat<T>& Quat<T>::operator-=(const Quat<T>& rhs)
{
	w -= rhs.w;
	v -= rhs.v;
	return *this;
}

template<typename T>
X_INLINE Quat<T>& Quat<T>::operator*=(const Quat<T> &rhs)
{
	Quat q = (*this) * rhs;
	v = q.v;
	w = q.w;
	return *this;
}

template<typename T>
X_INLINE Quat<T>& Quat<T>::operator*=(T rhs)
{
	w *= rhs;
	v *= rhs;
	return *this;
}

template<typename T>
X_INLINE Quat<T> Quat<T>::operator~() const
{
	return Quat<T>(w, -v.x, -v.y, -v.z);
}

template<typename T>
X_INLINE Quat<T> Quat<T>::operator-() const
{
	return Quat<T>(-w, -v.x, -v.y, -v.z);
}

template<typename T>
X_INLINE bool Quat<T>::compare(const Quat<T>& rhs, const T elipson) const
{
	const Quat<T>& lhs = *this;
	return (math<float>::abs(lhs.w - rhs.w) < elipson) && lhs.v.compare(rhs.v, elipson);
}

template<typename T>
X_INLINE bool Quat<T>::operator==(const Quat<T> &rhs) const
{
	const Quat<T>& lhs = *this;
	return (math<float>::abs(lhs.w - rhs.w) < EPSILON) && lhs.v == rhs.v;
}

template<typename T>
X_INLINE bool Quat<T>::operator!=(const Quat<T> &rhs) const
{
	return !(*this == rhs);
}