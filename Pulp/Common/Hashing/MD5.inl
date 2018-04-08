

X_NAMESPACE_BEGIN(core)

namespace Hash
{

    template<typename T>
    void MD5::update(const T& obj)
    {
        update(reinterpret_cast<const void*>(&obj), sizeof(T));
    }

    void MD5::update(const core::string& str)
    {
        update(reinterpret_cast<const void*>(str.data()), core::strUtil::StringBytes(str));
    }

    void MD5::update(const std::string& str)
    {
        update(reinterpret_cast<const void*>(str.data()), core::strUtil::StringBytes(str));
    }

    void MD5::update(const std::wstring& str)
    {
        update(reinterpret_cast<const void*>(str.data()), core::strUtil::StringBytes(str));
    }

} // namespace Hash

X_NAMESPACE_END