
X_NAMESPACE_BEGIN(core)

namespace Hash
{


    template<typename T>
    X_INLINE bool xxHash32::update(span<T> data)
    {
        static_assert(core::compileTime::IsPOD<T>::Value, "hashing of none POD type");
        return updateBytes(static_cast<const void*>(data.data()), data.size_bytes());
    }

    template<typename T>
    X_INLINE bool xxHash32::update(const T& type)
    {
        static_assert(core::compileTime::IsPOD<T>::Value, "hashing of none POD type");
        return updateBytes(static_cast<const void*>(&type), sizeof(T));
    }

    // ------------------------------------------------------------------------------

    template<typename T>
    X_INLINE bool xxHash64::update(span<T> data)
    {
        static_assert(core::compileTime::IsPOD<T>::Value, "hashing of none POD type");
        return updateBytes(static_cast<const void*>(data.data()), data.size_bytes());
    }

    template<typename T>
    X_INLINE bool xxHash64::update(const T& type)
    {
        static_assert(core::compileTime::IsPOD<T>::Value, "hashing of none POD type");
        return updateBytes(static_cast<const void*>(&type), sizeof(T));
    }


} // namespace Hash

X_NAMESPACE_END
