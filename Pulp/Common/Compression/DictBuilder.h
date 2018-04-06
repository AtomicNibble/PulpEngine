#pragma once

#include <Containers\Array.h>

X_NAMESPACE_BEGIN(core)

namespace Compression
{
    // creates a dictionary from the given sampler data.
    // sample sizes are size of each sample in sampleData in order.
    // Eg if sampleData is 200 bytes.
    // sampleSizes might contain: { 50, 25, 25, 75, 25 }

    static const size_t DICT_SAMPLER_SIZE_MAX = 128 * 1024; // 128kb.
    static const size_t DICT_SAMPLER_MIN_SAMPLES = 5;

    bool trainDictionary(const core::Array<uint8_t>& sampleData, const core::Array<size_t>& sampleSizes,
        core::Array<uint8_t>& dictOut, size_t maxDictSize);

} // namespace Compression

X_NAMESPACE_END