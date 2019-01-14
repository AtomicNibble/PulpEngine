#pragma once

#ifndef X_NAME_UTIL_H_
#define X_NAME_UTIL_H_

X_NAMESPACE_BEGIN(render)

namespace D3DDebug
{
    X_INLINE void SetDebugObjectName(ID3D12Object* pResource, const wchar_t* pName)
    {
#if X_DEBUG
        pResource->SetName(pName);
#else
        UNREFERENCED_PARAMETER(pResource);
        UNREFERENCED_PARAMETER(pName);
#endif
    }

    X_INLINE void SetDebugObjectName(ID3D12Object* pResource, const char* pName)
    {
#if X_DEBUG
        wchar_t wideName[256];
        core::strUtil::Convert(pName, wideName);

        pResource->SetName(wideName);
#else
        UNREFERENCED_PARAMETER(pResource);
        UNREFERENCED_PARAMETER(pName);
#endif
    }

    X_INLINE void SetDebugObjectName(ID3D12Object* pResource, core::string_view name)
    {
#if X_DEBUG
        wchar_t wideName[256];
        core::strUtil::Convert(name, wideName);

        pResource->SetName(wideName);
#else
        UNREFERENCED_PARAMETER(pResource);
        UNREFERENCED_PARAMETER(name);
#endif
    }

    template<size_t TNameLength>
    X_INLINE void SetDebugObjectName(ID3D12Object* pResource, const wchar_t (&name)[TNameLength])
    {
#if X_DEBUG
        pResource->SetName(name);
#else
        UNREFERENCED_PARAMETER(pResource);
        UNREFERENCED_PARAMETER(name);
#endif
    }

} // namespace D3DDebug

X_NAMESPACE_END

#endif // !X_NAME_UTIL_H_