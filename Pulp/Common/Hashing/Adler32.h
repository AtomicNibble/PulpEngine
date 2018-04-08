#pragma once

#ifndef X_HASH_ADLER32_H_
#define X_HASH_ADLER32_H_

#include <Util\Span.h>

X_NAMESPACE_BEGIN(core)

namespace Hash
{
    typedef uint32_t Adler32Val;

    Adler32Val Adler32(const char* str);
    Adler32Val Adler32(span<const char> buf);
    Adler32Val Adler32(span<const uint8_t> buf);
    Adler32Val Adler32(Adler32Val& adler, span<const uint8_t> buf);

} // namespace Hash

X_NAMESPACE_END

#endif // X_HASH_ADLER32_H_