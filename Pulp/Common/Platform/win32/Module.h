#pragma once

X_NAMESPACE_BEGIN(core)

namespace Module
{
    typedef void* Handle;
    typedef void* Proc;

    inline static Handle NULL_HANDLE;

    bool Supported(void);
    const char* Extension(void);

    Handle Load(const char* pName);
    Handle Load(const wchar_t* pName);

    Handle GetHandle(const char* pName);
    Handle GetHandle(const wchar_t* pName);

    Proc GetProc(Handle module, const char* pName);

    bool UnLoad(Handle module);

} // namespace Module

X_NAMESPACE_END