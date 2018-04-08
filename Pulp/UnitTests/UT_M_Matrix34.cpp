#include "stdafx.h"

X_USING_NAMESPACE;

using namespace core;

typedef ::testing::Types<float, double> MyTypes;
TYPED_TEST_CASE(Mat34, MyTypes);

template<typename T>
class Mat34 : public ::testing::Test
{
public:
};

template<typename T>
::std::ostream& operator<<(::std::ostream& os, const Matrix34<T>& m)
{
    os << "\n[" << m.m00 << ", " << m.m01 << ", " << m.m02 << ", " << m.m03 << "]";
    os << "\n[" << m.m10 << ", " << m.m11 << ", " << m.m12 << ", " << m.m13 << "]";
    os << "\n[" << m.m20 << ", " << m.m21 << ", " << m.m22 << ", " << m.m23 << "]";
    return os;
}

TYPED_TEST(Mat34, Contruction)
{
    typedef Matrix34<TypeParam> MatT;
    typedef TypeParam T;

    // Matrix34();
    {
        MatT c0(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
            0, 0, 0);
        MatT m0;

        EXPECT_EQ(c0, m0);
    }
    // Matrix34( T s );
    {
        MatT c0(
            3, 3, 3,
            3, 3, 3,
            3, 3, 3,
            3, 3, 3);

        MatT m0(3);

        EXPECT_EQ(c0, m0);
    }
    // Matrix34( const T *dt );
    {
        MatT c0(
            1, 2, 3,
            4, 5, 6,
            7, 8, 9,
            10, 11, 12);

        T data[12] = {
            (T)1, (T)2, (T)3,
            (T)4, (T)5, (T)6,
            (T)7, (T)8, (T)9,
            (T)10, (T)11, (T)12};

        MatT m0(data);

        EXPECT_EQ(c0, m0);
    }

    // Matrix34( T d0, T d1, T d2, T d3, T d4, T d5, T d6, T d7, T d8, T d9, T d10, T d12 );
    {
        MatT c0;
        c0.m00 = 1;
        c0.m01 = 4;
        c0.m02 = 7;
        c0.m10 = 2;
        c0.m11 = 5;
        c0.m12 = 8;
        c0.m20 = 3;
        c0.m21 = 6;
        c0.m22 = 9;

        c0.m03 = 10;
        c0.m13 = 11;
        c0.m23 = 12;

        MatT m0(
            1, 2, 3,
            4, 5, 6,
            7, 8, 9,
            10, 11, 12);

        EXPECT_EQ(c0, m0);
    }
    // Matrix34( const Vec3<T> &vx, const Vec3<T> &vy, const Vec3<T> &vz );
    {
        MatT c0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            0, 0, 0);

        MatT m0(Vec3<T>(1, 2, 3), Vec3<T>(5, 6, 7), Vec3<T>(9, 10, 11));

        EXPECT_EQ(c0, m0);
    }

    // Matrix34( const Vec3<T> &vx, const Vec3<T> &vy, const Vec3<T> &vz, const Vec3<T>& t);
    {
        MatT c0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m0(Vec3<T>(1, 2, 3), Vec3<T>(5, 6, 7), Vec3<T>(9, 10, 11),
            Vec3<T>(13, 14, 15));

        EXPECT_EQ(c0, m0);
    }

    // template< typename FromT >
    // Matrix34( const Matrix34<FromT>& src );
    {
        Matrix34<double> dmat0(
            1, 2, 3,
            4, 5, 6,
            7, 8, 9,
            10, 11, 12);

        Matrix34<float> fmat0(
            1, 2, 3,
            4, 5, 6,
            7, 8, 9,
            10, 11, 12);

        Matrix34<double> dmat1(fmat0);
        Matrix34<double> fmat1(dmat0);

        EXPECT_EQ(dmat0, dmat1);
        EXPECT_EQ(fmat0, fmat1);
    }

    // Matrix34( const Matrix22<T>& src );
    {
        MatT c0(
            1, 2, 0,
            3, 4, 0,
            0, 0, 1,
            0, 0, 0);

        Matrix22<T> mat22(
            1, 2,
            3, 4);

        MatT m0(mat22);

        EXPECT_EQ(c0, m0);
    }

    // Matrix34(const Matrix33<T>& m);
    {
        Matrix34<T> c0(
            1, 2, 0,
            3, 4, 0,
            0, 0, 1,
            0, 0, 0);

        Matrix33<T> mat33(
            1, 2, 0,
            3, 4, 0,
            0, 0, 1);

        MatT m0(mat33);

        EXPECT_EQ(c0, m0);
    }

    // Matrix34(const Matrix33<T>& m, const Vec3<T>& t);
    {
        Matrix34<T> c0(
            1, 2, 0,
            3, 4, 0,
            0, 0, 1,
            9, 10, 11);

        Matrix33<T> mat33(
            1, 2, 0,
            3, 4, 0,
            0, 0, 1);

        MatT m0(mat33, Vec3<T>(9, 10, 11));

        EXPECT_EQ(c0, m0);
    }

    // Matrix34( const Matrix34<T>& src );
    {
        MatT c0(
            1, 2, 3,
            4, 5, 6,
            7, 8, 9,
            10, 11, 12);

        Matrix34<T> mat34(
            1, 2, 3,
            4, 5, 6,
            7, 8, 9,
            10, 11, 12);

        MatT m0(mat34);

        EXPECT_EQ(c0, m0);
    }

    // Matrix34(const Quat<T>& q);
    {
        Matrix33<T> mat33(
            1, 2, 0,
            3, 4, 0,
            0, 0, 1);

        Quat<T> q(mat33);

        MatT m0 = q.toMatrix33();
        MatT m1(q);

        EXPECT_EQ(m0, m1);
    }
}

TYPED_TEST(Mat34, Assign)
{
    typedef Matrix34<TypeParam> MatT;
    typedef TypeParam T;

    // Matrix34<T>& operator=( const Matrix34<T>& rhs );
    {
        MatT c0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m0 = c0;

        EXPECT_EQ(c0, m0);
    }

    // Matrix34<T>& operator=( T rhs );
    {
        MatT c0(
            3, 3, 3,
            3, 3, 3,
            3, 3, 3,
            3, 3, 3);

        MatT m0 = MatT(3);

        EXPECT_EQ(c0, m0);
    }

    // template< typename FromT >
    // Matrix34<T>& operator=( const Matrix34<FromT>& rhs );
    {
        Matrix34<double> dmat0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        Matrix34<float> fmat0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        Matrix34<double> dmat1 = fmat0;
        Matrix34<double> fmat1 = dmat0;

        EXPECT_EQ(dmat0, dmat1);
        EXPECT_EQ(fmat0, fmat1);
    }

    // Matrix34<T>& operator=( const Matrix22<T>& rhs );
    {
        MatT c0(
            1, 2, 0,
            3, 4, 0,
            0, 0, 1,
            0, 0, 0);

        Matrix22<T> mat22(1, 2, 3, 4);

        MatT m0;
        m0 = mat22;

        EXPECT_EQ(c0, m0);
    }

    // Matrix34<T>& operator=( const Matrix33<T>& rhs );
    {
        MatT c0(
            1, 2, 3,
            4, 5, 6,
            7, 8, 9,
            0, 0, 0);

        Matrix33<T> mat33(
            1, 2, 3,
            4, 5, 6,
            7, 8, 9);

        MatT m0;
        m0 = mat33;

        EXPECT_EQ(c0, m0);
    }
}

TYPED_TEST(Mat34, Compare)
{
    typedef Matrix34<TypeParam> MatT;
    typedef TypeParam T;

    // bool equalCompare( const Matrix34<T>& rhs, T epsilon ) const;
    {
        T epsilon = (T)1.0e-04;

        MatT c0(
            (T)1.0e-04, (T)1.0e-04, (T)1.0e-04,
            (T)1.0e-04, (T)1.0e-04, (T)1.0e-04,
            (T)1.0e-04, (T)1.0e-04, (T)1.0e-04,
            (T)1.0e-04, (T)1.0e-04, (T)1.0e-04);

        MatT m0((T)3.0e-04, (T)3.0e-04, (T)3.0e-04,
            (T)3.0e-04, (T)3.0e-04, (T)3.0e-04,
            (T)3.0e-04, (T)3.0e-04, (T)3.0e-04,
            (T)1.0e-04, (T)1.0e-04, (T)1.0e-04);

        MatT m1((T)1.5e-04, (T)1.5e-04, (T)1.5e-04,
            (T)1.5e-04, (T)1.5e-04, (T)1.5e-04,
            (T)1.5e-04, (T)1.5e-04, (T)1.5e-04,
            (T)1.5e-04, (T)1.5e-04, (T)1.5e-04);

        EXPECT_FALSE(c0.equalCompare(m0, epsilon));
        EXPECT_TRUE(c0.equalCompare(m1, epsilon));
    }

    // bool operator==( const Matrix34<T> &rhs );
    {
        MatT c0(
            1, 2, 3,
            4, 5, 6,
            7, 8, 9,
            10, 11, (T)1.234e-06);

        MatT m0(
            1, 2, 3,
            4, 5, 6,
            7, 8, 9,
            10, 11, (T)1.235e-06);

        EXPECT_EQ(c0, m0);
    }

    // bool operator!=( const Matrix34<T> &rhs );
    {
        MatT c0(
            1, 2, 3,
            4, 5, 6,
            7, 8, 9,
            10, 11, (T)1.234e-06);

        MatT m0(
            2, 2, 3,
            4, 5, 6,
            7, 8, 9,
            10, 11, (T)1.235e-06);

        EXPECT_NE(c0, m0);
    }
}

TYPED_TEST(Mat34, Arithmetic)
{
    typedef Matrix34<TypeParam> MatT;
    typedef TypeParam T;

    // Matrix34<T>& operator*=( const Matrix34<T> &rhs );
    {
        MatT c0(
            20, 24, 28, // 5
            35, 42, 49, // 7
            65, 78, 91, // 9
                        //	125, 136, 147 // 11
            108, 128, 148);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m1(
            1, 2, 1,
            2, 3, 2,
            4, 5, 4,
            6, 7, 6);

        m0 *= m1;

        EXPECT_EQ(c0, m0);
    }

    // Matrix34<T>& operator+=( const Matrix34<T> &rhs );
    {
        MatT c0(
            2, 4, 6,
            10, 12, 14,
            18, 20, 22,
            26, 28, 30);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m1(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        m0 += m1;

        EXPECT_EQ(c0, m0);
    }

    // Matrix34<T>& operator-=( const Matrix34<T> &rhs );
    {
        MatT c0(
            0, 0, 0,
            0, 0, 0,
            0, 0, 0,
            0, 0, 0);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m1(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        m0 -= m1;

        EXPECT_EQ(c0, m0);
    }

    // Matrix34<T>& operator*=( T rhs );
    {
        MatT c0(
            2, 4, 6,
            10, 12, 14,
            18, 20, 22,
            26, 28, 30);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        m0 *= 2;

        EXPECT_EQ(c0, m0);
    }

    // Matrix34<T>& operator/=( T rhs );
    {
        MatT c0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m0(2, 4, 6,
            10, 12, 14,
            18, 20, 22,
            26, 28, 30);

        m0 /= 2;

        EXPECT_EQ(c0, m0);
    }

    // Matrix34<T>& operator+=( T rhs );
    {
        MatT c0(
            (T)1.025, (T)2.025, (T)3.025,
            (T)5.025, (T)6.025, (T)7.025,
            (T)9.025, (T)10.025, (T)11.025,
            (T)13.025, (T)14.025, (T)15.025);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        m0 += (T)0.025;

        EXPECT_EQ(c0, m0);
    }

    // Matrix34<T>& operator-=( T rhs );
    {
        MatT c0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m0(
            (T)1.025, (T)2.025, (T)3.025,
            (T)5.025, (T)6.025, (T)7.025,
            (T)9.025, (T)10.025, (T)11.025,
            (T)13.025, (T)14.025, (T)15.025);

        m0 -= (T)0.025;

        EXPECT_EQ(c0, m0);
    }

    // const Matrix34<T> operator*( const Matrix34<T> &rhs ) const;
    {
        MatT c0(
            20, 24, 28,
            35, 42, 49,
            65, 78, 91,
            //	125, 136, 147 // 11
            108, 128, 148);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m1(
            1, 2, 1,
            2, 3, 2,
            4, 5, 4,
            6, 7, 6);

        EXPECT_EQ(c0, (m0 * m1));
    }

    // const Matrix34<T> operator+( const Matrix34<T> &rhs ) const;
    {
        MatT c0(
            2, 4, 6,
            10, 12, 14,
            18, 20, 22,
            26, 28, 30);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m1(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        EXPECT_EQ(c0, (m0 + m1));
    }

    // const Matrix34<T> operator-( const Matrix34<T> &rhs ) const;
    {
        MatT c0(
            0, 0, 0,
            0, 0, 0,
            0, 0, 0,
            0, 0, 0);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m1(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        EXPECT_EQ(c0, (m0 - m1));
    }

    // const Vec3<T> operator*( const Vec3<T> &rhs ) const;
    {
        Vec3<T> cv0(1, 2, 3);

        MatT c0(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
            0, 0, 0);

        Vec3<T> diff = (cv0 - c0 * cv0);
        diff.x = fabs(diff.x);
        diff.y = fabs(diff.y);
        diff.z = fabs(diff.z);

        EXPECT_LE(diff.x, math<T>::EPSILON);
        EXPECT_LE(diff.y, math<T>::EPSILON);
        EXPECT_LE(diff.z, math<T>::EPSILON);
    }

    // const Matrix34<T> operator*( T rhs ) const;
    {
        MatT c0(
            2, 4, 6,
            10, 12, 14,
            18, 20, 22,
            26, 28, 30);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        EXPECT_EQ(c0, (m0 * (T)2));
    }

    // const Matrix34<T> operator/( T rhs ) const;
    {
        MatT c0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m0(2, 4, 6,
            10, 12, 14,
            18, 20, 22,
            26, 28, 30);

        EXPECT_EQ(c0, (m0 / (T)2));
    }

    // const Matrix34<T> operator+( T rhs ) const;
    {
        MatT c0(
            (T)1.025, (T)2.025, (T)3.025,
            (T)5.025, (T)6.025, (T)7.025,
            (T)9.025, (T)10.025, (T)11.025,
            (T)13.025, (T)14.025, (T)15.025);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        EXPECT_EQ(c0, (m0 + (T)0.025));
    }

    // const Matrix34<T> operator-( T rhs ) const;
    {
        MatT c0(1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m0(
            (T)1.025, (T)2.025, (T)3.025,
            (T)5.025, (T)6.025, (T)7.025,
            (T)9.025, (T)10.025, (T)11.025,
            (T)13.025, (T)14.025, (T)15.025);

        EXPECT_EQ(c0, (m0 - (T)0.025));
    }
}

TYPED_TEST(Mat34, GetSet)
{
    typedef Matrix34<TypeParam> MatT;
    typedef TypeParam T;

    // T& at( int row, int col );
    {
        MatT c0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m0;
        m0.at(0, 0) = 1;
        m0.at(1, 0) = 2;
        m0.at(2, 0) = 3;

        m0.at(0, 1) = 5;
        m0.at(1, 1) = 6;
        m0.at(2, 1) = 7;

        m0.at(0, 2) = 9;
        m0.at(1, 2) = 10;
        m0.at(2, 2) = 11;

        m0.at(0, 3) = 13;
        m0.at(1, 3) = 14;
        m0.at(2, 3) = 15;

        EXPECT_EQ(c0, m0);
    }

    // const T& at( int row, int col ) const;
    {
        MatT c0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        bool isOk = true;
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 4; ++c) {
                if (c0.at(r, c) != m0.at(r, c)) {
                    isOk = false;
                    break;
                }
            }
        }

        EXPECT_TRUE(isOk);
    }

    // Vec3<T> getColumn( int col ) const;
    {
        MatT c0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        EXPECT_EQ(c0.getColumn(0), Vec3<T>(1, 2, 3));
        EXPECT_EQ(c0.getColumn(1), Vec3<T>(5, 6, 7));
        EXPECT_EQ(c0.getColumn(2), Vec3<T>(9, 10, 11));
        EXPECT_EQ(c0.getColumn(3), Vec3<T>(13, 14, 15));
    }

    // void setColumn( int col, const Vec3<T> &v );
    {
        MatT c0(1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);
        MatT m0;
        m0.setToIdentity();

        m0.setColumn(0, Vec3<T>(1, 2, 3));
        m0.setColumn(1, Vec3<T>(5, 6, 7));
        m0.setColumn(2, Vec3<T>(9, 10, 11));
        m0.setColumn(3, Vec3<T>(13, 14, 15));

        EXPECT_EQ(c0, m0);
    }

    // Vec3<T> getRow( int row ) const;
    {
        MatT c0(1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        EXPECT_EQ(c0.getRow(0), Vec3<T>(1, 5, 9));
        EXPECT_EQ(c0.getRow(1), Vec3<T>(2, 6, 10));
        EXPECT_EQ(c0.getRow(2), Vec3<T>(3, 7, 11));
        // only 3 rows
    }

    // void setRow( int row, const Vec3<T> &v );
    {
        MatT c0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            0, 0, 0);
        MatT m0;
        m0.setToIdentity();

        m0.setRow(0, Vec3<T>(1, 5, 9));
        m0.setRow(1, Vec3<T>(2, 6, 10));
        m0.setRow(2, Vec3<T>(3, 7, 11));

        EXPECT_EQ(c0, m0);
    }

    // void getColumns( Vec3<T> *c0, Vec3<T> *c1, Vec3<T> *c2 ) const;
    {
        MatT c0(1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        Vec3<T> v0, v1, v2, v3;
        c0.getColumns(&v0, &v1, &v2, &v3);

        EXPECT_EQ(v0, Vec3<T>(1, 2, 3));
        EXPECT_EQ(v1, Vec3<T>(5, 6, 7));
        EXPECT_EQ(v2, Vec3<T>(9, 10, 11));
    }

    // void setColumns( const Vec3<T> &c0, const Vec3<T> &c1, const Vec3<T> &c2 );
    {
        MatT c0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);
        MatT m0;
        m0.setToIdentity();

        Vec3<T> v0(1, 2, 3);
        Vec3<T> v1(5, 6, 7);
        Vec3<T> v2(9, 10, 11);
        Vec3<T> v3(13, 14, 15);

        m0.setColumns(v0, v1, v2, v3);

        EXPECT_EQ(c0, m0);
    }

    // void getRows( Vec3<T> *r0, Vec3<T> *r1, Vec3<T> *r2 ) const;
    {
        MatT c0(1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            13, 14, 15);

        Vec3<T> v0, v1, v2;
        c0.getRows(&v0, &v1, &v2);

        EXPECT_EQ(v0, Vec3<T>(1, 5, 9));
        EXPECT_EQ(v1, Vec3<T>(2, 6, 10));
        EXPECT_EQ(v2, Vec3<T>(3, 7, 11));
    }

    // void setRows( const Vec3<T> &r0, const Vec3<T> &r1, const Vec3<T> &r2 );
    {
        MatT c0(1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            0, 0, 0);

        MatT m0;
        m0.setToIdentity();

        Vec3<T> v0(1, 5, 9);
        Vec3<T> v1(2, 6, 10);
        Vec3<T> v2(3, 7, 11);

        m0.setRows(v0, v1, v2);

        EXPECT_EQ(c0, m0);
    }
}

TYPED_TEST(Mat34, Misc)
{
    typedef Matrix34<TypeParam> MatT;
    typedef TypeParam T;

    // Matrix22<T> subMatrix22( int row, int col ) const;
    {
        MatT c0(
            1, 2, 3, 13,
            5, 6, 7, 14,
            9, 10, 11, 15,
            //	13, 14, 15,
            true);

        Matrix22<T> m22_00(
            1, 2,
            5, 6, true);

        Matrix22<T> m22_01(
            2, 3,
            6, 7, true);

        Matrix22<T> m22_10(
            5, 6,
            9, 10, true);

        Matrix22<T> m22_11(
            6, 7,
            10, 11, true);

        Matrix22<T> m22_02(
            3, 13,
            7, 14, true);

        Matrix22<T> m22_12(
            7, 14,
            11, 15, true);

        EXPECT_EQ(m22_00, c0.subMatrix22(0, 0));
        EXPECT_EQ(m22_01, c0.subMatrix22(0, 1));
        EXPECT_EQ(m22_10, c0.subMatrix22(1, 0));
        EXPECT_EQ(m22_11, c0.subMatrix22(1, 1));

        EXPECT_EQ(m22_02, c0.subMatrix22(0, 2));
        EXPECT_EQ(m22_12, c0.subMatrix22(1, 2));
    }
    // void setToNull();
    {
        MatT c0(
            0, 0, 0,
            0, 0, 0,
            0, 0, 0,
            0, 0, 0);

        MatT m0;
        m0.setToNull();

        EXPECT_EQ(c0, m0);
    }
    // void setToIdentity();
    {
        MatT c0(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
            0, 0, 0);

        MatT m0;
        m0.setToIdentity();

        EXPECT_EQ(c0, m0);
    }
    // T determinant() const;
    {
        MatT m0(
            (T)0.009505, (T)0.848196, (T)-0.529596, (T)1.0,
            (T)-0.999570, (T)0.022748, (T)0.018494, (T)1.0,
            (T)0.027734, (T)0.529193, (T)0.848048, (T)1.0,
            true);

        T det = m0.determinant();

        T diff = fabs((T)0.999999 - det);

        EXPECT_LE(diff, math<T>::EPSILON);
    }
    // T trace() const;
    {
        MatT m0(
            (T)0.009505, (T)0.848196, (T)-0.529596, (T)1.0,
            (T)-0.999570, (T)0.022748, (T)0.018494, (T)1.0,
            (T)0.027734, (T)0.529193, (T)0.848048, (T)1.0,
            true);

        T trace = m0.trace();

        T diff = fabs((T)0.880301 - trace);

        EXPECT_LE(diff, math<T>::EPSILON);
    }

    // Matrix33<T> diagonal() const;
    {
        MatT c0(
            1, 0, 0,
            0, 6, 0,
            0, 0, 11,
            10, 20, 30);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            10, 20, 30);

        EXPECT_EQ(c0, m0.diagonal());
    }

    // Matrix33<T> lowerTriangular() const;
    {
        MatT c0(
            1, 0, 0, 10,
            5, 6, 0, 20,
            9, 10, 11, 30,
            true);

        MatT m0(
            1, 2, 3, 10,
            5, 6, 7, 20,
            9, 10, 11, 30,
            true);

        EXPECT_EQ(c0, m0.lowerTriangular());
    }

    // Matrix33<T> upperTriangular() const;
    {
        MatT c0(
            1, 2, 3, 10,
            0, 6, 7, 20,
            0, 0, 11, 30,
            true);

        MatT m0(
            1, 2, 3, 10,
            5, 6, 7, 20,
            9, 10, 11, 30,
            true);

        EXPECT_EQ(c0, m0.upperTriangular());
    }
}

TYPED_TEST(Mat34, Transpose)
{
    typedef Matrix34<TypeParam> MatT;
    typedef TypeParam T;

    // void transpose();
    {
        MatT c0(
            1, 5, 9,
            2, 6, 10,
            3, 7, 11,
            10, 20, 30);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            10, 20, 30);

        m0.transpose();

        EXPECT_EQ(c0, m0);
    }
    // Matrix33<T> transposed() const;
    {
        MatT c0(
            1, 5, 9,
            2, 6, 10,
            3, 7, 11,
            10, 20, 30);

        MatT m0(
            1, 2, 3,
            5, 6, 7,
            9, 10, 11,
            10, 20, 30);

        EXPECT_EQ(c0, m0.transposed());
    }
}

TYPED_TEST(Mat34, Invert)
{
    typedef Matrix34<TypeParam> MatT;
    typedef TypeParam T;

    // void invert (T epsilon = math<T>::EPSILON );
    {
        MatT c0(
            (T)0.009505, (T)-0.999570, (T)0.027734,
            (T)0.848197, (T)0.022749, (T)0.529193,
            (T)-0.529597, (T)0.018494, (T)0.848048,
            (T)1.0, (T)1.0, (T)1.0);

        MatT m0(
            (T)0.009505, (T)0.848196, (T)-0.529596,
            (T)-0.999570, (T)0.022748, (T)0.018494,
            (T)0.027734, (T)0.529193, (T)0.848048,
            (T)1.0, (T)1.0, (T)1.0);

        m0.invert();

        EXPECT_EQ(c0, m0);
    }
    // Matrix33<T> inverted( T epsilon = math<T>::EPSILON ) const;
    {
        MatT c0(
            (T)0.009505, (T)-0.999570, (T)0.027734,
            (T)0.848197, (T)0.022749, (T)0.529193,
            (T)-0.529597, (T)0.018494, (T)0.848048,
            (T)1.0, (T)1.0, (T)1.0);

        MatT m0(
            (T)0.009505, (T)0.848196, (T)-0.529596,
            (T)-0.999570, (T)0.022748, (T)0.018494,
            (T)0.027734, (T)0.529193, (T)0.848048,
            (T)1.0, (T)1.0, (T)1.0);

        bool single = (c0 == m0.inverted());
        /*
		bool multi = true;
		Vec3<T> da((T)0.015, (T)0.034, (T)-0.025);
		Vec3<T> eu(0, 0, 0);

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
		*/
    }
}

TYPED_TEST(Mat34, PrePostMultiply)
{
    typedef Matrix34<TypeParam> MatT;
    typedef TypeParam T;

    // Vec3<T> preMultiply( const Vec3<T> &v ) const;
    {
        Vec3<T> cv0(1, 2, 3);

        MatT c0(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
            10, 20, 30);

        Vec3<T> diff = cv0 - c0.preMultiply(cv0);

        diff.x = fabs(diff.x);
        diff.y = fabs(diff.y);
        diff.z = fabs(diff.z);

        EXPECT_LE(diff.x, math<T>::EPSILON);
        EXPECT_LE(diff.y, math<T>::EPSILON);
        EXPECT_LE(diff.z, math<T>::EPSILON);
    }
    // Vec3<T> preMultiply( const Vec3<T> &v ) const;
    {
        Vec3<T> cv0(1, 2, 3);

        MatT c0(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
            10, 20, 30);

        Vec3<T> diff = cv0 - c0.preMultiply(cv0);

        diff.x = fabs(diff.x);
        diff.y = fabs(diff.y);
        diff.z = fabs(diff.z);

        EXPECT_LE(diff.x, math<T>::EPSILON);
        EXPECT_LE(diff.y, math<T>::EPSILON);
        EXPECT_LE(diff.z, math<T>::EPSILON);
    }
    // Vec3<T> postMultiply( const Vec3<T> &v ) const;
    {
        Vec3<T> cv0(1, 2, 3);

        MatT c0(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
            10, 20, 30);

        Vec3<T> diff = cv0 - c0.postMultiply(cv0);

        diff.x = fabs(diff.x);
        diff.y = fabs(diff.y);
        diff.z = fabs(diff.z);

        EXPECT_LE(diff.x, math<T>::EPSILON);
        EXPECT_LE(diff.y, math<T>::EPSILON);
        EXPECT_LE(diff.z, math<T>::EPSILON);
    }
    // Vec3<T> postMultiply( const Vec3<T> &v ) const;
    {
        Vec3<T> cv0(1, 2, 3);

        MatT c0(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
            10, 20, 30);

        Vec3<T> diff = cv0 - c0.postMultiply(cv0);

        diff.x = fabs(diff.x);
        diff.y = fabs(diff.y);
        diff.z = fabs(diff.z);

        EXPECT_LE(diff.x, math<T>::EPSILON);
        EXPECT_LE(diff.y, math<T>::EPSILON);
        EXPECT_LE(diff.z, math<T>::EPSILON);
    }
}

TYPED_TEST(Mat34, Transform)
{
    typedef Matrix34<TypeParam> MatT;
    typedef TypeParam T;

    // Vec3<T> transformVec( const Vec3<T> &rhs ) const;
    {
        MatT c0(
            (T)0.45399, (T)-0.89101, (T)0.00000,
            (T)0.89101, (T)0.45399, (T)0.00000,
            (T)0.00000, (T)0.00000, (T)1.00000,
            (T)1.0, (T)1.0, (T)1.0,
            true);

        MatT m0(
            (T)0.45399, (T)-0.89101, (T)0.00000,
            (T)0.89101, (T)0.45399, (T)0.00000,
            (T)0.00000, (T)0.00000, (T)1.00000,
            (T)1.0, (T)1.0, (T)1.0,
            true);

        Vec3<T> v0((T)1.25, (T)-4.312, (T)5.2112);

        EXPECT_EQ(c0.transformVec(v0), m0.transformVec(v0));
    }

    // Matrix33<T> invertTransform() const;
    // TODO
    /*
	{
		MatT c0(
			(T) 0.45399, (T) 0.89101, (T) 0.00000, (T)1.0,
			(T)-0.89101, (T) 0.45399, (T) 0.00000, (T)1.0,
			(T) 0.00000, (T) 0.00000, (T) 1.00000, (T)1.0,
			true
			);

		MatT m0(
			(T) 0.45399, (T)-0.89101, (T) 0.00000, (T)1.0,
			(T) 0.89101, (T) 0.45399, (T) 0.00000, (T)1.0,
			(T) 0.00000, (T) 0.00000, (T) 1.00000, (T)1.0,
			true
			);

		m0 = m0.invertTransform();
		
		EXPECT_EQ(c0, m0);
	}
	*/
}

TYPED_TEST(Mat34, Rotate)
{
    typedef Matrix34<TypeParam> MatT;
    typedef TypeParam T;

    // void rotate( const Vec3<T> &axis, T radians );
    {
        MatT c0(
            (T)1.000000000000000, (T)0.000000000000000, (T)0.000000000000000, (T)1.0,
            (T)0.000000000000000, (T)0.840963470480837, (T)-0.541091897293636, (T)1.0,
            (T)0.000000000000000, (T)0.541091897293636, (T)0.840963470480837, (T)1.0,
            true);

        MatT m0;
        m0.setToIdentity();
        m0.rotate(Vec3<T>(1, 0, 0), toRadians((T)32.758));
        m0.setTranslate(Vec3<T>(1, 1, 1));

        EXPECT_EQ(c0, m0);
    }

    // void rotate( const Vec3<T> &eulerRadians );
    {
        MatT c0(
            (T)0.091142486334637, (T)0.428791685462663, (T)-0.898794046299167,
            (T)-0.730490309010667, (T)0.642199351624628, (T)0.232301315567534,
            (T)0.676813826414798, (T)0.635387821138395, (T)0.371759816444386,
            (T)1.0, (T)1.0, (T)1.0);

        Vec3<T> eu0;
        eu0.x = toRadians((T)32);
        eu0.y = toRadians((T)64);
        eu0.z = toRadians((T)78);
        MatT m0;
        m0.setToIdentity();
        m0.rotate(eu0);
        m0.setTranslate(Vec3<T>(1, 1, 1));

        EXPECT_EQ(c0, m0);
    }

    // void rotate( const Vec3<T> &from, const Vec3<T> &to, const Vec3<T> &worldUp );
    {
        MatT c0((T)1.000000000000000, (T)0.000000000000000, (T)0.000000000000000, (T)1.0,
            (T)0.000000000000000, (T)0.840963470480837, (T)-0.541091897293636, (T)1.0,
            (T)0.000000000000000, (T)0.541091897293636, (T)0.840963470480837, (T)1.0,
            true);

        MatT m0;
        m0.setToIdentity();
        m0.rotate(Vec3<T>(1, 0, 0), toRadians((T)32.758));
        m0.setTranslate(Vec3<T>(1, 1, 1));

        EXPECT_EQ(c0, m0);
    }
}

TYPED_TEST(Mat34, StaticUtil)
{
    typedef Matrix34<TypeParam> MatT;
    typedef TypeParam T;

    // static Matrix33<T> identity();
    {
        MatT c0(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
            0, 0, 0);

        EXPECT_EQ(c0, MatT::identity());
    }

    // static Matrix33<T> one();
    {
        MatT c0(
            1, 1, 1,
            1, 1, 1,
            1, 1, 1,
            1, 1, 1);

        EXPECT_EQ(c0, MatT::one());
    }

    // static Matrix33<T> zero();
    {
        MatT c0(
            0, 0, 0,
            0, 0, 0,
            0, 0, 0,
            0, 0, 0);

        EXPECT_EQ(c0, MatT::zero());
    }

    // static Matrix33<T> createRotation( const Vec3<T> &axis, T radians );
    {
        MatT c0(
            (T)1.000000000000000, (T)0.000000000000000, (T)0.000000000000000, (T)0.0,
            (T)0.000000000000000, (T)0.840963470480837, (T)-0.541091897293636, (T)0.0,
            (T)0.000000000000000, (T)0.541091897293636, (T)0.840963470480837, (T)0.0,
            true);

        MatT m0 = MatT::createRotation(Vec3<T>(1, 0, 0), toRadians((T)32.758));

        EXPECT_EQ(c0, m0);
    }

    // static Matrix33<T> createRotation( const Vec3<T> &from, const Vec3<T> &to, const Vec3<T> &worldUp );
    {
        MatT c0((T)0.707106781186548, (T)0.000000000000000, (T)-0.707106781186548, (T)0.0,
            (T)0.000000000000000, (T)1.000000000000000, (T)0.000000000000000, (T)0.0,
            (T)0.707106781186548, (T)0.000000000000000, (T)0.707106781186548, (T)0.0,
            true);

        MatT m0 = MatT::createRotation(
            Vec3<T>((T)0.00000000000000, (T)0.000000000000000, (T)1.00000000000000),
            Vec3<T>((T)0.70710678118654, (T)0.000000000000000, (T)0.70710678118654),
            Vec3<T>((T)0.00000000000000, (T)1.000000000000000, (T)0.00000000000000));

        EXPECT_EQ(c0, m0);
    }

    // static Matrix33<T> createRotation( const Vec3<T> &eulerRadians );
    {
        MatT c0(
            (T)0.091142486334637, (T)0.428791685462663, (T)-0.898794046299167,
            (T)-0.730490309010667, (T)0.642199351624628, (T)0.232301315567534,
            (T)0.676813826414798, (T)0.635387821138395, (T)0.371759816444386,
            (T)0.0, (T)0.0, (T)0.0);

        Vec3<T> eu0;
        eu0.x = toRadians((T)32);
        eu0.y = toRadians((T)64);
        eu0.z = toRadians((T)78);
        MatT m0 = MatT::createRotation(Vec3<T>(eu0));

        EXPECT_EQ(c0, m0);
    }

    // static Matrix33<T> createScale( T s );
    {
        MatT c0(
            6, 0, 0,
            0, 6, 0,
            0, 0, 6,
            0, 0, 0);

        EXPECT_EQ(c0, MatT::createScale((T)6));
    }

    // static Matrix33<T> createScale( const Vec2<T> &v );
    {
        MatT c0(
            2, 0, 0,
            0, 3, 0,
            0, 0, 1,
            0, 0, 0);

        EXPECT_EQ(c0, MatT::createScale(Vec2<T>(2, 3)));
    }

    // static Matrix33<T> createScale( const Vec3<T> &v );
    {
        MatT c0(
            2, 0, 0,
            0, 3, 0,
            0, 0, 4,
            0, 0, 0);

        EXPECT_EQ(c0, MatT::createScale(Vec3<T>(2, 3, 4)));
    }

    // static Matrix33<T> alignZAxisWithTarget( Vec3<T> targetDir, Vec3<T> upDir );
    {
        MatT c0(
            1, 0, 0,
            0, 1, 0,
            0, 0, -1,
            0, 0, 0);

        MatT m0 = MatT::alignZAxisWithTarget(Vec3<T>(0, 0, -1), Vec3<T>(0, 1, 0));

        EXPECT_EQ(c0, m0);
    }
}