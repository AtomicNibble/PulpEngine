#include "EngineCommon.h"
#include "Module.h"

X_NAMESPACE_BEGIN(core)

namespace Module
{
    bool Supported(void)
    {
        return true;
    }

    const char* Extension(void)
    {
        return ".dll";
    }

    Handle Load(const char* pName)
    {
        return ::LoadLibraryA(pName);
    }

    Handle Load(const wchar_t* pName)
    {
        return ::LoadLibraryW(pName);
    }

    Handle GetHandle(const char* pName)
    {
        return ::GetModuleHandleA(pName);
    }

    Handle GetHandle(const wchar_t* pName)
    {
        return ::GetModuleHandleW(pName);
    }

    Proc GetProc(Handle module, const char* pName)
    {
        return reinterpret_cast<Proc>(::GetProcAddress(reinterpret_cast<HMODULE>(module), pName));
    }

    bool UnLoad(Handle module)
    {
        return ::FreeLibrary(reinterpret_cast<HMODULE>(module)) == TRUE;
    }

} // namespace Module

X_NAMESPACE_END
