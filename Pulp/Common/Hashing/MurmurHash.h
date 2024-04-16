#pragma once

#ifndef _X_HASH_MURMUR_H
#define _X_HASH_MURMUR_H

X_NAMESPACE_BEGIN(core)

namespace Hash
{
    // Calculates a hash of the given data, fullinfo -> to https://sites.google.com/site/murmurhash/
    //
    // Extremely simple - compiles down to ~52 instructions on x86.
    // Excellent distribution - Passes chi-squared tests for practically all keysets & bucket sizes.
    // Excellent avalanche behavior - Maximum bias is under 0.5%.
    // Excellent collision resistance - Passes Bob Jenkin's frog.c torture-test. No collisions possible for 4-byte keys, no small (1- to 7-bit) differentials.
    // Excellent performance - measured on an Intel Core 2 Duo @ 2.4 ghz


    typedef uint32_t MurmurHash2Val;

    MurmurHash2Val MurmurHash2(const void* pKey, size_t numBytes, uint32_t seed);

}; // namespace Hash

X_NAMESPACE_END

#endif // _X_HASH_MURMUR_H