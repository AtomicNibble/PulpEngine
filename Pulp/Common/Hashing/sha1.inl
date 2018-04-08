

X_NAMESPACE_BEGIN(core)

namespace Hash
{

    template<typename T>
    void SHA1::update(const T& obj)
    {
        update(reinterpret_cast<const void*>(&obj), sizeof(T));
    }

    void SHA1::update(const core::string& str)
    {
        update(reinterpret_cast<const void*>(str.data()), core::strUtil::StringBytes(str));
    }

    void SHA1::update(const std::string& str)
    {
        update(reinterpret_cast<const void*>(str.data()), core::strUtil::StringBytes(str));
    }

    void SHA1::update(const std::wstring& str)
    {
        update(reinterpret_cast<const void*>(str.data()), core::strUtil::StringBytes(str));
    }

} // namespace Hash

X_NAMESPACE_END