#include "EngineCommon.h"
#include "DictBuilder.h"

#define ZDICT_STATIC_LINKING_ONLY
#include <../../3rdparty/source/zstd-1.2.0/lib/dictBuilder/zdict.h>

#include <Util\Cpu.h>

X_LINK_LIB("libzstd" X_PP_IF(X_DEBUG, "d", ""));

X_NAMESPACE_BEGIN(core)

namespace Compression
{
    bool trainDictionary(const core::Array<uint8_t>& sampleData, const core::Array<size_t>& sampleSizes,
        core::Array<uint8_t>& dictOut, size_t maxDictSize)
    {
        core::CpuInfo cpu;

        COVER_params_t params;
        core::zero_object(params);
        params.k = 0;
        params.d = 12;
        params.steps = 128;
        params.nbThreads = cpu.GetCoreCount();

        if (sampleSizes.size() < DICT_SAMPLER_MIN_SAMPLES) {
            X_ERROR("Dict", "Atleast %" PRIuS " samples required. %" PRIuS " provided", DICT_SAMPLER_MIN_SAMPLES, sampleSizes.size());
            return false;
        }

        for (const auto sampleSize : sampleSizes) {
            if (sampleSize > DICT_SAMPLER_SIZE_MAX) {
                X_ERROR("Dict", "A sample was provided that exceeds the max per sample size of %" PRIuS " sample size. %" PRIuS,
                    DICT_SAMPLER_SIZE_MAX, sampleSize);
                return false;
            }
        }

        dictOut.resize(maxDictSize);

        size_t res = COVER_optimizeTrainFromBuffer(
            dictOut.data(),
            maxDictSize,
            sampleData.data(),
            sampleSizes.data(),
            safe_static_cast<uint32_t>(sampleSizes.size()),
            &params);

        if (ZDICT_isError(res)) {
            dictOut.clear();
            X_ERROR("Dict", "Failed to create dictionary. Err(%" PRIuS "): %s", res, ZDICT_getErrorName(res));
            return false;
        }

        dictOut.resize(res);
        return true;
    }

} // namespace Compression

X_NAMESPACE_END