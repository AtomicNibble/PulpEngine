#pragma once


template<typename T>
Quat<T> QuatLookAt(const Vec3<T>& direction, const Vec3<T>& up)
{
    Vec3<T> cols[3];

    cols[2] = -direction.normalized();
    cols[0] = up.cross(cols[2]).normalized();
    cols[1] = cols[2].cross(cols[0]);

    Matrix33<T> mat(cols[0], cols[1], cols[2]);

    return Quat<T>(mat);
}
