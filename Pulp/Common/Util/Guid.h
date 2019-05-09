#pragma once

X_NAMESPACE_BEGIN(core)

class Guid
{
public:
    using GuidByteArr = std::array<uint8_t, 16>;
    using GuidStr = core::StackString<48, char>;

    struct GUID
    {
        uint32_t data1;
        uint16_t data2;
        uint16_t data3;
        uint8_t data4[8];
    };

public:
    Guid();
    explicit Guid(const GuidByteArr& bytes);
    template<size_t N>
    explicit Guid(const uint8_t (&bytes)[N]);
    explicit Guid(core::string_view str);

    Guid(const Guid &other) = default;
    Guid &operator=(const Guid &other) = default;
    Guid(Guid &&other) = default;
    Guid &operator=(Guid &&other) = default;

    bool operator==(const Guid &other) const;
    bool operator!=(const Guid &other) const;

    const GuidByteArr& bytes(void) const;
    const GUID& guid(void) const;
    bool isValid(void) const;

    const char* toString(GuidStr& buf) const;
    static Guid newGuid(void);

private:
    union U
    {
        U() :
            bytes{ {0} }
        {}
        U(const GuidByteArr& bytes) :
            bytes(bytes)
        {}

        GuidByteArr bytes;
        GUID guid;
    } data_;
};


template<size_t N>
inline Guid::Guid(const uint8_t(&bytes)[N])
{
    static_assert(N == 16, "Invalid number of bytes");
    std::memcpy(data_.bytes.data(), bytes, data_.bytes.size());
}


X_NAMESPACE_END
