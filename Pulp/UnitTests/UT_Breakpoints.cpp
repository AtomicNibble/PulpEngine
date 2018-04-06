#include "stdafx.h"

#include <Containers\FixedFifo.h>

#include <Debugging\HardwareBreakpoint.h>
#include <Debugging\ExceptionHandler.h>

// toggle them
#define ENABLE_BREAKPOINT_TESTS 0

X_USING_NAMESPACE;
using namespace core;

#define BREAKPOINT_TEST(bits, type, flag)                                     \
    TEST(BreakPoint, Read##bits)                                              \
    {                                                                         \
        type var = 0;                                                         \
        type* varAddress = &var;                                              \
                                                                              \
        hardwareBP::Install(varAddress, hardwareBP::Condition::READ_OR_WRITE, \
            hardwareBP::Size::flag);                                          \
                                                                              \
        EXPECT_DEATH({ type test = *varAddress; }, "");                       \
                                                                              \
        hardwareBP::Uninstall(varAddress);                                    \
    }                                                                         \
    TEST(BreakPoint, Write##bits)                                             \
    {                                                                         \
        type var[2] = {};                                                     \
        hardwareBP::Install(var, hardwareBP::Condition::WRITE,                \
            hardwareBP::Size::flag);                                          \
        EXPECT_DEATH({ var[0] = 0; }, "");                                    \
        hardwareBP::Uninstall(var);                                           \
    }

#if ENABLE_BREAKPOINT_TESTS

BREAKPOINT_TEST(8, uint8_t, BYTE_1)
BREAKPOINT_TEST(16, uint16_t, BYTE_2)
BREAKPOINT_TEST(32, uint32_t, BYTE_4)
BREAKPOINT_TEST(64, uint64_t, BYTE_8)

#endif
