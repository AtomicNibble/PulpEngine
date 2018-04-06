

template<typename T>
void SHA512::update(const T& obj)
{
    update(reinterpret_cast<const void*>(&obj), sizeof(T));
}

void SHA512::update(const core::string& str)
{
    update(reinterpret_cast<const void*>(str.data()), core::strUtil::StringBytes(str));
}

void SHA512::update(const std::string& str)
{
    update(reinterpret_cast<const void*>(str.data()), core::strUtil::StringBytes(str));
}

void SHA512::update(const std::wstring& str)
{
    update(reinterpret_cast<const void*>(str.data()), core::strUtil::StringBytes(str));
}