#pragma once

#ifndef X_HASH_ADLER32_H_
#define X_HASH_ADLER32_H_

X_NAMESPACE_BEGIN(core)

namespace Hash
{
    typedef uint32_t Adler32Val;

    Adler32Val Adler32(const char* str);
    Adler32Val Adler32(const char* str, size_t length);
    Adler32Val Adler32(const void* buf, size_t length);
    Adler32Val Adler32(Adler32Val& adler, const void* buf, size_t length);

} // namespace Hash

X_NAMESPACE_END

#endif // X_HASH_ADLER32_H_