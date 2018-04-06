#pragma once

#include <ICompression.h>

X_NAMESPACE_BEGIN(core)

namespace Compression
{
    X_DISABLE_WARNING(4324) // padded for alignment.

    class CompressorAlloc
    {
        typedef ICompressor ICompressor;
        typedef Algo Algo;

    public:
        CompressorAlloc(Algo::Enum algo);
        ~CompressorAlloc();

        X_INLINE ICompressor& operator*()
        {
            return *get();
        }
        X_INLINE ICompressor* operator->()
        {
            return get();
        }
        X_INLINE ICompressor* get(void)
        {
            return reinterpret_cast<ICompressor*>(buffer_);
        }

    private:
        Algo::Enum algo_;
        X_ALIGNED_SYMBOL(char buffer_[32], 16);
    };

    X_ENABLE_WARNING(4324)

} // namespace Compression

X_NAMESPACE_END