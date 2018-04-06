#pragma once

X_NAMESPACE_DECLARE(core,
                    struct XFile)

X_NAMESPACE_BEGIN(render)

namespace shader
{
    X_DECLARE_ENUM(BufferType)
    (
        Structured,
        RWStructured);

    class Buffer
    {
    public:
        Buffer() = default;
        Buffer(const char* pName, int16_t bindPoint, int16_t bindCount, BufferType::Enum type);
        Buffer(core::string& name, int16_t bindPoint, int16_t bindCount, BufferType::Enum type);

        X_INLINE const core::string& getName(void) const;
        X_INLINE int16_t getBindPoint(void) const;
        X_INLINE int16_t getBindCount(void) const;
        X_INLINE BufferType::Enum getType(void) const;

        bool SSave(core::XFile* pFile) const;
        bool SLoad(core::XFile* pFile);

    private:
        core::string name_;
        int16_t bindPoint_;
        int16_t bindCount_;
        BufferType::Enum type_;
    };

    X_INLINE const core::string& Buffer::getName(void) const
    {
        return name_;
    }

    X_INLINE int16_t Buffer::getBindPoint(void) const
    {
        return bindPoint_;
    }

    X_INLINE int16_t Buffer::getBindCount(void) const
    {
        return bindCount_;
    }

    X_INLINE BufferType::Enum Buffer::getType(void) const
    {
        return type_;
    }

} // namespace shader

X_NAMESPACE_END
