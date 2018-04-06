#include "EngineCommon.h"
#include "CompressorAlloc.h"

#include "LZ4.h"
#include "LZ5.h"
#include "Zlib.h"
#include "lzma2.h"
#include "Store.h"

X_NAMESPACE_BEGIN(core)

namespace Compression
{
    static_assert(Algo::ENUM_COUNT == 7, "Added additional compression algos? this code needs updating.");

    namespace
    {
        const size_t MAX_COMPRESSOR_SIZE = core::Max<size_t>(
                                               core::Max(
                                                   core::Max(
                                                       core::Max(
                                                           core::Max(
                                                               core::Max(
                                                                   core::Max(
                                                                       sizeof(core::Compression::Compressor<core::Compression::LZ4>),
                                                                       sizeof(core::Compression::Compressor<core::Compression::LZ4HC>)),
                                                                   sizeof(core::Compression::Compressor<core::Compression::LZ5>)),
                                                               sizeof(core::Compression::Compressor<core::Compression::LZ5HC>)),
                                                           sizeof(core::Compression::Compressor<core::Compression::LZMA>)),
                                                       sizeof(core::Compression::Compressor<core::Compression::Zlib>)),
                                                   sizeof(core::Compression::Compressor<core::Compression::Store>)),
                                               16)
                                           + 16;

        const size_t MAX_COMPRESSOR_ALIGN = core::Max<size_t>(
            core::Max(
                core::Max(
                    core::Max(
                        core::Max(
                            core::Max(
                                core::Max(
                                    X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ4>),
                                    X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ4HC>)),
                                X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ5>)),
                            X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZ5HC>)),
                        X_ALIGN_OF(core::Compression::Compressor<core::Compression::LZMA>)),
                    X_ALIGN_OF(core::Compression::Compressor<core::Compression::Zlib>)),
                X_ALIGN_OF(core::Compression::Compressor<core::Compression::Store>)),
            16);

    } // namespace

    CompressorAlloc::CompressorAlloc(Algo::Enum algo) :
        algo_(algo)
    {
        static_assert(sizeof(buffer_) >= MAX_COMPRESSOR_SIZE, "Buffer can't fit all compressors");
        X_ASSERT_ALIGNMENT(buffer_, MAX_COMPRESSOR_ALIGN, 0);

        using core::Compression::Compressor;

        switch (algo) {
            case core::Compression::Algo::LZ4:
                core::Mem::Construct<Compressor<core::Compression::LZ4>>(buffer_);
                break;
            case core::Compression::Algo::LZ4HC:
                core::Mem::Construct<Compressor<core::Compression::LZ4HC>>(buffer_);
                break;
            case core::Compression::Algo::LZ5:
                core::Mem::Construct<Compressor<core::Compression::LZ5>>(buffer_);
                break;
            case core::Compression::Algo::LZ5HC:
                core::Mem::Construct<Compressor<core::Compression::LZ5HC>>(buffer_);
                break;
            case core::Compression::Algo::LZMA:
                core::Mem::Construct<Compressor<core::Compression::LZMA>>(buffer_);
                break;
            case core::Compression::Algo::ZLIB:
                core::Mem::Construct<Compressor<core::Compression::Zlib>>(buffer_);
                break;
            case core::Compression::Algo::STORE:
                core::Mem::Construct<Compressor<core::Compression::Store>>(buffer_);
                break;
            default:
                X_ASSERT_UNREACHABLE();
                break;
        }
    }

    CompressorAlloc::~CompressorAlloc()
    {
        Mem::Destruct(reinterpret_cast<ICompressor*>(buffer_));
    }

} // namespace Compression

X_NAMESPACE_END