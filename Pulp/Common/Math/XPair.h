#pragma once

#ifndef _X_MATH_PAIR_H_
#define _X_MATH_PAIR_H_

template<typename T>
struct Pair
{
    inline Pair(T x, T y);

    inline Pair<T> operator+(const Pair<T>& other);
    inline Pair<T> operator-(const Pair<T>& other);

    inline void set(T x, T y);

public:
    T x;
    T y;
};

#include "XPair.inl"

typedef Pair<int> Pairi;
typedef Pair<float> Pairf;
typedef Pair<double> Paird;

#endif // !_X_MATH_PAIR_H_
