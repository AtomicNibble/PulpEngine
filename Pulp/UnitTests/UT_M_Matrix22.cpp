#include "stdafx.h"



X_USING_NAMESPACE;

using namespace core;

typedef ::testing::Types<float, double> MyTypes;
TYPED_TEST_CASE(Mat22, MyTypes);

template <typename T>
class Mat22 : public ::testing::Test {
public:
};

TYPED_TEST(Mat22, Contruction) {

	typedef Matrix22<TypeParam> MatT;
	typedef TypeParam T;


	// Matrix22();
	{	
		MatT c0(1, 0, 0, 1);
		EXPECT_EQ(c0, MatT());

	}
	// explicit Matrix22( T s );
	{	
		MatT c0(3, 3, 3, 3);
		EXPECT_EQ(c0, MatT(3));
	}
	// Matrix22( const T *dt );
	{		
		T data[4] = { 1, 2, 3, 4 };
		MatT c0(1, 2, 3, 4);
		EXPECT_EQ(c0, MatT(data));

	}
	// Matrix22( T d0, T d1, T d2, T d3 );
	{		
		T data[4] = { 1, 2, 3, 4 };
		MatT c0(1, 2, 3, 4);
		EXPECT_EQ(data[0], c0.m[0]);
		EXPECT_EQ(data[1], c0.m[1]);
		EXPECT_EQ(data[2], c0.m[2]);
		EXPECT_EQ(data[3], c0.m[3]);
	}
	// Matrix22( const Vec2<T> &vx, const Vec2<T> &vy );
	{	
		MatT c0(1, 2, 3, 4);
		EXPECT_EQ(c0, MatT(Vec2<T>(1, 2), Vec2<T>(3, 4)));

	}
	// template< typename FromT > Matrix22( const Matrix22<FromT>& src );
	{	
		Matrix22<double> dmat0(1, 2, 3, 4);
		Matrix22<float>  fmat0(1, 2, 3, 4);
		EXPECT_EQ(dmat0, Matrix22<double>(fmat0));
		EXPECT_EQ(fmat0, Matrix22<float>(dmat0));

	}
	// Matrix22( const Matrix22<T>& src );
	{	
		MatT c0(1, 2, 3, 4);
		MatT m0(c0);
		EXPECT_EQ(c0, m0);
	}
	// operator T*() { return (T*)m; }
	{	
		MatT c0(1, 2, 3, 4);
		T *data = (T*)c0;
		EXPECT_EQ(c0.m[0], data[0]);
		EXPECT_EQ(c0.m[1], data[1]);
		EXPECT_EQ(c0.m[2], data[2]);
		EXPECT_EQ(c0.m[3], data[3]);
	}
	// operator const T*() const { return (const T*)m; }
	{		
		MatT c0(1, 2, 3, 4);
		const T *const_data = (const T *)c0;
		EXPECT_EQ(c0.m[0], const_data[0]);
		EXPECT_EQ(c0.m[1], const_data[1]);
		EXPECT_EQ(c0.m[2], const_data[2]);
		EXPECT_EQ(c0.m[3], const_data[3]);

	}
	// Matrix22<T>& operator=( const Matrix22<T>& rhs );
	{	
		MatT c0(1, 2, 3, 4);
		MatT m0 = c0;
		EXPECT_EQ(c0, m0);
	}
	// Matrix22<T>& operator=( T rhs );
	{	
		MatT c0(4, 4, 4, 4);
		MatT m0 = MatT(static_cast<T>(4));
		EXPECT_EQ(c0, m0);
	}
	// template< typename FromT > Matrix22<T>& operator=( const Matrix22<FromT>& rhs );
	{	
		Matrix22<double> dmat0(1, 2, 3, 4);
		Matrix22<float>  fmat0(1, 2, 3, 4);
		Matrix22<double> dmat1;
		Matrix22<float>  fmat1;

		dmat1 = fmat0;
		fmat1 = dmat0;

		EXPECT_EQ(dmat0, dmat1);
		EXPECT_EQ(fmat0, fmat1);

	}
	// bool equalCompare( const Matrix22<T>& rhs, T epsilon ) const;
	{	
		T epsilon = (T)1.0e-05;
		MatT c0((T)1.0e-04, (T)1.0e-04, (T)1.0e-04, (T)1.0e-04);
		MatT m0((T)1.3e-04, (T)1.3e-04, (T)1.3e-04, (T)1.3e-04);
		MatT m1((T)1.01e-04, (T)1.01e-04, (T)1.01e-04, (T)1.01e-04);
		EXPECT_FALSE(m0.equalCompare(c0, epsilon));
		EXPECT_TRUE(m1.equalCompare(c0, epsilon));

	}
	// bool operator==( const Matrix22<T> &rhs ) const;
	{	
		MatT c0((T)1.2345, (T)2.3456, (T)3.4567, (T)4.5678);
		MatT m0((T)1.2345, (T)2.3456, (T)3.4567, (T)4.5678);
		EXPECT_EQ(c0, m0);
	}
	// bool operator!=( const Matrix22<T> &rhs ) const;
	{	
		MatT c0((T)1.2345, (T)2.3456, (T)3.4567, (T)4.5678);
		MatT m0((T)1.2345, (T)2.3456, (T)3.4567, (T)4.5678);
		EXPECT_FALSE((c0 != m0));

	}
	// Matrix22<T>& operator*=( const Matrix22<T> &rhs );
	{	
		MatT c0(23, 34, 31, 46);
		MatT m0(1, 2, 3, 4);
		MatT m1(5, 6, 7, 8);
		m0 *= m1;
		EXPECT_EQ(c0, m0);

	}
	// Matrix22<T>& operator+=( const Matrix22<T> &rhs );
	{	
		MatT c0(6, 8, 10, 12);
		MatT m0(1, 2, 3, 4);
		MatT m1(5, 6, 7, 8);
		m0 += m1;
		EXPECT_EQ(c0, m0);

	}
	// Matrix22<T>& operator-=( const Matrix22<T> &rhs );
	{	
		MatT c0((T)-4.2, (T)-4.9, (T)-4.3, (T)-4.1);
		MatT m0(1, 2, 3, 4);
		MatT m1((T)5.2, (T)6.9, (T)7.3, (T)8.1);
		m0 -= m1;
		EXPECT_EQ(c0, m0);

	}
	// Matrix22<T>& operator*=( T s );
	{	
		MatT c0((T)3.03, (T)6.03, (T)-9.03, (T)12.03);
		MatT m0((T)1.01, (T)2.01, (T)-3.01, (T)4.01);
		m0 *= (T)3;
		EXPECT_EQ(c0, m0);

	}
	// Matrix22<T>& operator/=( T s );
	{	
		MatT c0((T)1.01, (T)2.01, (T)-3.01, (T)4.01);
		MatT m0((T)3.03, (T)6.03, (T)-9.03, (T)12.03);
		m0 /= (T)3;
		EXPECT_EQ(c0, m0);

	}
	// Matrix22<T>& operator+=( T s );
	{	
		MatT c0((T)8.053, (T)11.053, (T)-4.007, (T)17.053);
		MatT m0((T)3.03, (T)6.03, (T)-9.03, (T)12.03);
		m0 += (T)5.023;
		EXPECT_EQ(c0, m0);

	}
	// Matrix22<T>& operator-=( T s );
	{	
		MatT c0((T)-1.993, (T)1.007, (T)-14.053, (T)7.007);
		MatT m0((T)3.03, (T)6.03, (T)-9.03, (T)12.03);
		m0 -= (T)5.023;
		EXPECT_EQ(c0, m0);

	}
	// const Matrix22<T> operator*( const Matrix22<T> &rhs ) const;
	{	
		MatT c0(23, 34, 31, 46);
		MatT m0(1, 2, 3, 4);
		MatT m1(5, 6, 7, 8);
		EXPECT_EQ(c0, (m0*m1));

	}
	// const Matrix22<T> operator+( const Matrix22<T> &rhs ) const;
	{	
		MatT c0(6, 8, 10, 12);
		MatT m0(1, 2, 3, 4);
		MatT m1(5, 6, 7, 8);
		EXPECT_EQ(c0, (m0 + m1));

	}
	// const Matrix22<T> operator-( const Matrix22<T> &rhs ) const;
	{	
		MatT c0((T)-4.2, (T)-4.9, (T)-4.3, (T)-4.1);
		MatT m0(1, 2, 3, 4);
		MatT m1((T)5.2, (T)6.9, (T)7.3, (T)8.1);
		EXPECT_EQ(c0, (m0 - m1));

	}
	// const Vec2<T> operator*( const Vec2<T> &rhs ) const;
	{	
		Vec2<T> cv0((T)-28.395060, (T)-37.037040);
		MatT m0(1, 2, 3, 4);
		Vec2<T> v0((T)1.23456, (T)-9.87654);
		Vec2<T> diff = (cv0 - (m0*v0));
		EXPECT_LT(fabs(diff.x), EPSILON);
		EXPECT_LT(fabs(diff.y), EPSILON);

	}
	// const Matrix22<T> operator*( T rhs ) const;
	{	
		MatT c0((T)3.03, (T)6.03, (T)-9.03, (T)12.03);
		MatT m0((T)1.01, (T)2.01, (T)-3.01, (T)4.01);
		EXPECT_EQ(c0, (m0*(T)3));

	}
	// const Matrix22<T> operator/( T rhs ) const;
	{	
		MatT c0((T)1.01, (T)2.01, (T)-3.01, (T)4.01);
		MatT m0((T)3.03, (T)6.03, (T)-9.03, (T)12.03);
		EXPECT_EQ(c0, (m0 / (T)3));

	}
	// const Matrix22<T> operator+( T rhs ) const;
	{
		MatT c0((T)8.053, (T)11.053, (T)-4.007, (T)17.053);
		MatT m0((T)3.03, (T)6.03, (T)-9.03, (T)12.03);
		EXPECT_EQ(c0, (m0 + (T)5.023));

	}
	// const Matrix22<T> operator-( T rhs ) const;
	{	
		MatT c0((T)-1.993, (T)1.007, (T)-14.053, (T)7.007);
		MatT m0((T)3.03, (T)6.03, (T)-9.03, (T)12.03);
		EXPECT_EQ(c0, (m0 - (T)5.023));

	}
	// T& at( int row, int col );
	{
		MatT c0(1, 2, 3, 4);
		MatT m0;
		m0.at(0, 0) = 1;
		m0.at(1, 0) = 2;
		m0.at(0, 1) = 3;
		m0.at(1, 1) = 4;
		EXPECT_EQ(c0, m0);

	}
	// const T& at( int row, int col ) const;
	{
		MatT c0(1, 2, 3, 4);
		EXPECT_LT(fabs(1 - c0.at(0, 0)), EPSILON);
		EXPECT_LT(fabs(2 - c0.at(1, 0)), EPSILON);
		EXPECT_LT(fabs(3 - c0.at(0, 1)), EPSILON);
		EXPECT_LT(fabs(4 - c0.at(1, 1)), EPSILON);

	}
	// Vec2<T> getColumn( int col ) const;
	{	
		MatT m0(1, 2, 3, 4);
		EXPECT_EQ(Vec2<T>((T)1, (T)2), m0.getColumn(0));
		EXPECT_EQ(Vec2<T>((T)3, (T)4), m0.getColumn(1));
	}
	// void setColumn( int col, const Vec2<T> &v );
	{	
		MatT c0(1, 2, 3, 4);
		MatT m0;
		m0.setColumn(0, Vec2<T>(1, 2));
		m0.setColumn(1, Vec2<T>(3, 4));
		EXPECT_EQ(c0, m0);
	}
	// Vec2<T> getRow( int row ) const;
	{	
		MatT m0(1, 2, 3, 4);
		EXPECT_EQ(Vec2<T>((T)1, (T)3), m0.getRow(0));
		EXPECT_EQ(Vec2<T>((T)2, (T)4), m0.getRow(1));
	}
	// void setRow( int row, const Vec2<T> &v );
	{	
		MatT c0(1, 2, 3, 4);
		MatT m0;
		m0.setRow(0, Vec2<T>(1, 3));
		m0.setRow(1, Vec2<T>(2, 4));
		EXPECT_EQ(c0, m0);
	}
	// void getColumns( Vec2<T> *c0, Vec2<T> *c1 ) const;
	{
		Vec2<T> cv0(1, 2);
		Vec2<T> cv1(3, 4);
		MatT m0(1, 2, 3, 4);
		Vec2<T> v0, v1;
		m0.getColumns(&v0, &v1);
		EXPECT_EQ(cv0, v0);
		EXPECT_EQ(cv1, v1);
	}
	// void setColumns( const Vec2<T> &c0, const Vec2<T> &c1 );
	{
		MatT c0(1, 2, 3, 4);
		MatT m0;
		m0.setColumn(0, Vec2<T>(1, 2));
		m0.setColumn(1, Vec2<T>(3, 4));
		EXPECT_EQ(c0, m0);
	}
	// void getRows( Vec2<T> *r0, Vec2<T> *r1 ) const;
	{	
		Vec2<T> cv0(1, 3);
		Vec2<T> cv1(2, 4);
		MatT m0(1, 2, 3, 4);
		Vec2<T> v0, v1;
		m0.getRows(&v0, &v1);
		EXPECT_EQ(cv0, v0);
		EXPECT_EQ(cv1, v1);

	}
	// void setRows( const Vec2<T> &r0, const Vec2<T> &r1 );
	{	
		MatT c0(1, 2, 3, 4);
		MatT m0;
		m0.setRows(Vec2<T>(1, 3), Vec2<T>(2, 4));
		EXPECT_EQ(c0, m0);

	}
	// void setToNull();
	{	
		MatT c0(0, 0, 0, 0);
		MatT m0(1, 2, 3, 4);
		m0.setToNull();
		EXPECT_EQ(c0, m0);

	}
	// void setToIdentity();
	{	
		MatT c0(1, 0, 0, 1);
		MatT m0(1, 2, 3, 4);
		m0.setToIdentity();
		EXPECT_EQ(c0, m0);

	}
	// T determinant() const;
	{	
		MatT m0(1, 2, 3, 4);
		T det = m0.determinant();
		EXPECT_LT(fabs(det - (T)-2.0), EPSILON);
	}
	// T trace() const;
	{	
		MatT m0(1, 2, 3, 4);
		T trace = m0.trace();
		EXPECT_LT(fabs(trace - (T)5.0), EPSILON);
	}
	// Matrix22<T> diagonal() const;
	{	
		MatT c0(1, 0, 0, 4);
		MatT m0(1, 2, 3, 4);
		EXPECT_EQ(c0, m0.diagonal());

	}
	// Matrix22<T> lowerTriangular() const;
	{	
		MatT c0(1, 2, 0, 4);
		MatT m0(1, 2, 3, 4);
		EXPECT_EQ(c0, m0.lowerTriangular());

	}
	// Matrix22<T> upperTriangular() const;
	{	
		MatT c0(1, 0, 3, 4);
		MatT m0(1, 2, 3, 4);
		EXPECT_EQ(c0, m0.upperTriangular());

	}
	// void transpose();
	{	
		MatT c0(1, 3, 2, 4);
		MatT m0(1, 2, 3, 4);
		m0.transpose();
		EXPECT_EQ(c0, m0);

	}
	// Matrix22<T> transposed() const;
	{	
		MatT c0(1, 3, 2, 4);
		MatT m0(1, 2, 3, 4);
		EXPECT_EQ(c0, m0.transposed());

	}
	// void invert (T epsilon = EPSILON );
	{	
		MatT c0(-2, 1, (T)1.5, (T)-0.5);
		MatT m0(1, 2, 3, 4);
		m0.invert();
		EXPECT_EQ(c0, m0);

	}
	// Matrix22<T> inverted( T epsilon = EPSILON ) const;
	{
		MatT c0(-2, 1, (T)1.5, (T)-0.5);
		MatT m0(1, 2, 3, 4);

		bool single = (c0 == m0.inverted());

		bool multi = true;
		T da = (T)0.0015;
		T eu = 0;

		int iter = 100000;
		for (int i = 0; i < iter; ++i) {
			MatT m0 = MatT::createRotation(eu);
			MatT m1 = m0.inverted();
			MatT res0 = m0*m1;
			MatT res1 = m1.inverted();
			if (!(MatT::identity() == res0 && res1 == m0)) {
				multi = false;
				break;
			}
			eu += da;
		}

		EXPECT_TRUE(single && multi);
	}
	// Vec2<T> preMultiply( const Vec2<T> &v ) const;
	{	
		Vec2<T> cv0((T)-18.518520, (T)-35.802480);
		MatT m0(1, 2, 3, 4);
		Vec2<T> v0((T)1.23456, (T)-9.87654);
		Vec2<T> diff = (cv0 - m0.preMultiply(v0));
		EXPECT_LE(fabs(diff.x), EPSILON);
		EXPECT_LE(fabs(diff.y), EPSILON);

	}
	// Vec2<T> postMultiply( const Vec2<T> &v ) const;
	{	
		Vec2<T> cv0((T)-28.395060, (T)-37.037040);
		MatT m0(1, 2, 3, 4);
		Vec2<T> v0((T)1.23456, (T)-9.87654);
		Vec2<T> diff = (cv0 - m0.postMultiply(v0));
		EXPECT_LE(fabs(diff.x), EPSILON);
		EXPECT_LE(fabs(diff.y), EPSILON);
	}
	// Vec2<T> transformVec( const Vec2<T> &rhs );
	{	
		Vec2<T> cv0((T)-28.395060, (T)-37.037040);
		MatT m0(1, 2, 3, 4);
		Vec2<T> v0((T)1.23456, (T)-9.87654);
		Vec2<T> diff = (cv0 - m0.transformVec(v0));
		EXPECT_LE(fabs(diff.x), EPSILON);
		EXPECT_LE(fabs(diff.y), EPSILON);
	}
	// void rotate( T radians );
	{	
		MatT c0((T) 2.182514, (T)-0.486452,
			(T) 4.606950, (T)-1.943200);
		MatT m0(1, 2, 3, 4);
		m0.rotate(toRadians((T)76));
		EXPECT_EQ(c0, m0);
	}
	// Matrix22<T> invertTransform() const;
	{	
		MatT c0((T)1.02, (T)3.04, (T)2.03, (T)4.05);
		MatT m0((T)1.02, (T)2.03, (T)3.04, (T)4.05);
		EXPECT_EQ(c0, m0.invertTransform());
	}
	// static Matrix22<T> identity();
	{	
		MatT c0(1, 0, 0, 1);
		EXPECT_EQ(c0, MatT::identity());
	}
	// static Matrix22<T> one();
	{	
		MatT c0(1, 1, 1, 1);
		EXPECT_EQ(c0, MatT::one());
	}
	// static Matrix22<T> zero();
	{	
		MatT c0(0, 0, 0, 0);
		EXPECT_EQ(c0, MatT::zero());
	}
	// static Matrix22<T> createRotation( T radians );
	{	
		MatT c0((T) 0.241922, (T)-0.970296,
			(T) 0.970296, (T) 0.241922);
		EXPECT_EQ(c0, MatT::createRotation(toRadians((T)76)));
	}
	// static Matrix22<T> createScale( T s );
	{	
		MatT c0(3, 0, 0, 3);
		EXPECT_EQ(c0, MatT::createScale(3));
	}
	// static Matrix22<T> createScale( const Vec2<T> &v );
	{		
		MatT c0(3, 0, 0, 4);
		EXPECT_EQ(c0, MatT::createScale(Vec2<T>(3, 4)));
	}

}