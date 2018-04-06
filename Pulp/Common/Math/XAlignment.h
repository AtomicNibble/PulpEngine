#pragma once

#ifndef _X_MATH_ALIGNMENT_H_
#define _X_MATH_ALIGNMENT_H_

namespace internal
{
    /// \ingroup Types
    /// \brief A struct representing possible alignment modes, used in conjunction with the flags class.
    struct Alignment
    {
        /// \brief The number of alignment modes.
        static const unsigned int FLAGS_COUNT = 8;

        /// \brief The different alignment modes.
        enum Enum
        {
            LEFT_ALIGN = (1u << 0),   ///< Aligns at the left border.
            LEFT_DOCK = (1u << 1),    ///< Docks to the left border.
            TOP_ALIGN = (1u << 2),    ///< Aligns at the top border.
            TOP_DOCK = (1u << 3),     ///< Docks to the top border.
            RIGHT_ALIGN = (1u << 4),  ///< Aligns at the right border.
            RIGHT_DOCK = (1u << 5),   ///< Docks to the right border.
            BOTTOM_ALIGN = (1u << 6), ///< Aligns at the bottom border.
            BOTTOM_DOCK = (1u << 7)   ///< Docks to the bottom border.
        };

        /// \brief Internal helper class.
        struct Bits
        {
            uint32_t LEFT_ALIGN : 1;
            uint32_t LEFT_DOCK : 1;
            uint32_t TOP_ALIGN : 1;
            uint32_t TOP_DOCK : 1;
            uint32_t RIGHT_ALIGN : 1;
            uint32_t RIGHT_DOCK : 1;
            uint32_t BOTTOM_ALIGN : 1;
            uint32_t BOTTOM_DOCK : 1;
        };

        /// \brief Internal helper function returning a human-readable string for the value given.
        static const char* ToString(uint32_t value)
        {
            switch (value) {
                case LEFT_ALIGN:
                    return "LEFT_ALIGN";

                case LEFT_DOCK:
                    return "LEFT_DOCK";

                case TOP_ALIGN:
                    return "TOP_ALIGN";

                case TOP_DOCK:
                    return "TOP_DOCK";

                case RIGHT_ALIGN:
                    return "RIGHT_ALIGN";

                case RIGHT_DOCK:
                    return "RIGHT_DOCK";

                case BOTTOM_ALIGN:
                    return "BOTTOM_ALIGN";

                case BOTTOM_DOCK:
                    return "BOTTOM_DOCK";

                default:
                    X_NO_SWITCH_DEFAULT;
            }
        }
    };
} // namespace internal

#include "Util/Flags.h"

typedef internal::Alignment Alignment;
typedef Flags<Alignment> AlignmentFlags;

X_DECLARE_FLAG_OPERATORS(AlignmentFlags);

#endif // !_X_MATH_ALIGNMENT_H_
