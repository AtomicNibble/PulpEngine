#pragma once

#ifndef _X_MATH_ALIGNMENT_H_
#define _X_MATH_ALIGNMENT_H_

X_DECLARE_FLAGS(Alignment)(
    LEFT_ALIGN,
    LEFT_DOCK,
    TOP_ALIGN,
    TOP_DOCK,
    RIGHT_ALIGN,
    RIGHT_DOCK,
    BOTTOM_ALIGN,
    BOTTOM_DOCK
);

#include "Util/Flags.h"

typedef Flags<Alignment> AlignmentFlags;

X_DECLARE_FLAG_OPERATORS(AlignmentFlags);

#endif // !_X_MATH_ALIGNMENT_H_
