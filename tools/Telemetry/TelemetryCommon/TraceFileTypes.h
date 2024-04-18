#pragma once

#include "Types.h"

#define TELEM_TAG(a, b, c, d) (tt_uint32)((d << 24) | (c << 16) | (b << 8) | a)


static const char* TRACE_FILE_EXTENSION = "trace";
static const tt_uint32 TRACR_FILE_FOURCC = TELEM_TAG('t', 'r', 'a', 'c');
static const tt_uint8 TRACE_FILE_VERSION = 1;

// write a header.
struct TelemFileHdr
{
    tt_uint32 fourCC;
    tt_uint8 version;
    tt_uint8 _pad[3 + 8];

    bool isValid(void) const {
        return fourCC == TRACR_FILE_FOURCC;
    }
};

static_assert(sizeof(TelemFileHdr) == 16, "Size changed");

