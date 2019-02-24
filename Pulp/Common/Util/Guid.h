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



X_NAMESPACE_END