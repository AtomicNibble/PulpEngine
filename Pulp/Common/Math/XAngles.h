#pragma once


// The z axis is up.A pure yaw rotates about the z axis.
// The x axis is forward.A pure roll rotates about the x axis.The y axis extends out to your left.
// A pure pitch rotates about the y axis.


template<typename T>
class Angles
{
public:
	X_DECLARE_ENUM(Rotation)(
		PITCH,
		YAW,
		ROLL
	);

	typedef core::StackString256 StrBuf;

	typedef T	TYPE;
	typedef T	value_type;
	//
	static const size_t DIM = 3;
	static const size_t MEM_LEN = sizeof(T)*DIM;

	typedef Angles<T> MyT;

public:
	Angles();
	Angles(T pitch, T yaw, T roll);
	explicit Angles(const Vec3<T>& v);
	explicit Angles(const Matrix33<T>& m);

	void set(T pitch, T yaw, T roll);

	void setRoll(T roll);
	void setPitch(T pitch);
	void setYaw(T yaw);

	T roll(void) const;
	T pitch(void) const;
	T yaw(void) const;

	T		operator[](size_t index) const;
	T&		operator[](size_t index);
	MyT		operator-() const;							// negate angles, in general not the inverse rotation
	MyT&	operator=(const MyT& a);
	MyT		operator+(const MyT& a) const;
	MyT&	operator+=(const MyT& a);
	MyT		operator-(const MyT& a) const;
	MyT&	operator-=(const MyT& a);
	MyT		operator*(const T a) const;
	MyT&	operator*=(const T a);
	MyT		operator/(const T a) const;
	MyT&	operator/=(const T a);

	bool	compare(const MyT& a) const;						// exact compare, no epsilon
	bool	compare(const MyT& a, const T epsilon) const;		// compare with epsilon
	bool	operator==(const MyT& a) const;						// exact compare, no epsilon
	bool	operator!=(const MyT& a) const;						// exact compare, no epsilon

	MyT&	normalize360(void);
	MyT&	normalize180(void);

	void	clamp(const Angles& min, const Angles& max);

	Vec3<T>			toVec3(void) const;
	Vec3<T>			toVec3Radians(void) const;
	Vec3<T>			toForward(void) const;
	Quat<T>			toQuat(void) const;
	Matrix33<T>		toMat3(void) const;
	Matrix44<T>		toMat4(void) const;
	Vec3<T>			toAngularVelocity(void) const;
	const char*		toString(StrBuf& buf) const;

	static MyT zero(void);

private:
	T pitch_;
	T yaw_;
	T roll_;
};

typedef Angles<float> Anglesf;
typedef Angles<double> Anglesd;


#include "XAngles.inl"