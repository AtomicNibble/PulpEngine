#pragma once

X_NAMESPACE_BEGIN(core)

namespace Hash
{
    struct DigestBase
    {
    protected:
        static const char* ToString(char* pBuf, size_t bufSize, const uint8_t* pDigest, size_t numBytes);
    };

    template<size_t NumBytes>
    struct Digest : public DigestBase
    {
        typedef char String[(NumBytes + 1) * 2];

        static const int32_t NUM_BYTES = NumBytes;
        static const int32_t NUM_BITS = NumBytes * 8;

    public:
        X_INLINE Digest()
        {
            zero_this(this);
        }
        X_INLINE Digest(const std::array<uint8_t, NumBytes>& arr)
        {
            std::memcpy(bytes, arr.data(), arr.size());
        }

        X_INLINE const char* ToString(String& buf) const
        {
            return DigestBase::ToString(buf, sizeof(buf), bytes, NumBytes);
        }

        X_INLINE bool operator==(const Digest& oth) const
        {
            return std::memcmp(data, oth.data, sizeof(data)) == 0;
        }
        X_INLINE bool operator!=(const Digest& oth) const
        {
            return !(*this == oth);
        }

        X_INLINE void clear(void)
        {
            zero_this(this);
        }

        union
        {
            uint8_t bytes[NumBytes];
            uint32_t data[NumBytes / 4];
        };
    };

} // namespace Hash

X_NAMESPACE_END