
X_NAMESPACE_BEGIN(game)

namespace weapon
{
    namespace Util
    {
        X_INLINE WeaponClass::Enum WeaponClassFromStr(const char* pStr)
        {
            return WeaponClassFromStr(pStr, pStr + core::strUtil::strlen(pStr));
        }

        X_INLINE InventoryType::Enum InventoryTypeFromStr(const char* pStr)
        {
            return InventoryTypeFromStr(pStr, pStr + core::strUtil::strlen(pStr));
        }

        X_INLINE FireType::Enum FireTypeFromStr(const char* pStr)
        {
            return FireTypeFromStr(pStr, pStr + core::strUtil::strlen(pStr));
        }

        X_INLINE AmmoCounterStyle::Enum AmmoCounterStyleFromStr(const char* pStr)
        {
            return AmmoCounterStyleFromStr(pStr, pStr + core::strUtil::strlen(pStr));
        }

    } // namespace Util
} // namespace weapon

X_NAMESPACE_END