#pragma once

#ifndef _X_MATH_PAIR_H_
#define _X_MATH_PAIR_H_

/// \ingroup Types
/// \brief A simple class representing a pair of arithmetic types.
/// \details This class is a simple helper class that supports storing arithmetic types that can be acted upon
/// component-wise.
/// \sa xRect
template<typename T>
struct Pair
{
    /// Constructs a pair from given values.
    inline Pair(T x, T y);

    /// Adds another pair of values component-wise.
    inline Pair<T> operator+(const Pair<T>& other);

    /// Subtracts another pair of values component-wise.
    inline Pair<T> operator-(const Pair<T>& other);

    /// Sets the values of the pair.
    inline void Set(T x, T y);

    T x;
    T y;
};

#include "XPair.inl"

typedef Pair<int> Pairi;
typedef Pair<float> Pairf;
typedef Pair<double> Paird;

#endif // !_X_MATH_PAIR_H_
