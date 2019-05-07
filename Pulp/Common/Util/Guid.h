#pragma once

X_NAMESPACE_BEGIN(core)

class Guid
{
public:
    using GuidByteArr = std::array<uint8_t, 16>;

    using GuidStr = core::StackString<48, char>;

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
    bool isValid(void) const;

    const char* toString(GuidStr& buf) const;
    static Guid newGuid(void);

private:
    GuidByteArr bytes_;
};


template<size_t N>
inline Guid::Guid(const uint8_t(&bytes)[N])
{
    static_assert(N == 16, "Invalid number of bytes");
    std::memcpy(bytes_.data(), bytes, bytes_.size());
}


X_NAMESPACE_END
