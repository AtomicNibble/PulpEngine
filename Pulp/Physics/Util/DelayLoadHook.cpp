#include "stdafx.h"
#include "DelayLoadHook.h"

#include <Platform\Module.h>

#if X_PLATFORM_WIN32
#include <delayimp.h>

#pragma comment(lib, "delayimp")

#endif //!X_PLATFORM_WIN32

X_NAMESPACE_BEGIN(physics)

DelayLoadHook gDelayLoadHook;

DelayLoadHook::DelayLoadHook() :
    config_(Config::Normal)
{
}

void DelayLoadHook::forceConfig(Config config)
{
    config_ = config;
}

DelayLoadHook::Config DelayLoadHook::getConfig(void) const
{
    return config_;
}

const char* DelayLoadHook::getPhysXCommonDEBUGDllName(void) const
{
    return getCommonDllName(Config::Debug);
}

const char* DelayLoadHook::getPhysXCommonCHECKEDDllName(void) const
{
    return getCommonDllName(Config::Checked);
}

const char* DelayLoadHook::getPhysXCommonPROFILEDllName(void) const
{
    return getCommonDllName(Config::Profile);
}

const char* DelayLoadHook::getPhysXCommonDllName(void) const
{
    return getCommonDllName(Config::Release);
}

const char* DelayLoadHook::getPhysxName(const char* pRequestedName)
{
    // we need to work out which dll it is.
    // before we can override it.
    // we will have thinks like:
    // * Physx
    // * PhysXCommon
    // * PhysXCooking
    // * PhysXCharacterKinematic

    if (config_ == Config::Normal) {
        return pRequestedName;
    }

    const char* pNameEnd = pRequestedName + core::strUtil::strlen(pRequestedName);

    // the order of these is important.
    // anything that is a substring of anotehr should come later.
    if (core::strUtil::Find(pRequestedName, pNameEnd, "PhysXCharacterKinematic") == pRequestedName) {
        return createNameForConfig("PhysXCharacterKinematic", dllNames_[NameBuffer::PhysxCharacterKinematic]);
    }
    else if (core::strUtil::Find(pRequestedName, pNameEnd, "PhysXCooking") == pRequestedName) {
        return createNameForConfig("PhysXCooking", dllNames_[NameBuffer::PhysxCooking]);
    }
    else if (core::strUtil::Find(pRequestedName, pNameEnd, "PhysXCommon") == pRequestedName) {
        return createNameForConfig("PhysXCommon", dllNames_[NameBuffer::PhysxCommon]);
    }
    else if (core::strUtil::Find(pRequestedName, pNameEnd, "PhysX") == pRequestedName) {
        return createNameForConfig("PhysX", dllNames_[NameBuffer::Physx]);
    }

    // this should not happen tbh, so tell me about it when it does.
    X_ASSERT_UNREACHABLE();
    return pRequestedName;
}

const char* DelayLoadHook::createNameForConfig(const char* pPrefix, core::Path<char>& buffer)
{
    X_ASSERT(config_ != Config::Normal, "Create config name should only be called when overiding")();

    buffer.clear();
    buffer.append(pPrefix);

    switch (config_) {
        case Config::Debug:
            buffer.append("DEBUG");
            break;
        case Config::Checked:
            buffer.append("CHECKED");
            break;
        case Config::Profile:
            buffer.append("PROFILE");
            break;
        case Config::Release:
            break;
    }

    buffer.setExtension("dll");
    return buffer.c_str();
}

const char* DelayLoadHook::getCommonDllName(Config config) const
{
    switch (config) {
        case Config::Debug:
            return "PhysXCommonDEBUG.dll";
        case Config::Checked:
            return "PhysXCommonCHECKED.dll";
        case Config::Profile:
            return "PhysXCommonPROFILE.dll";
        case Config::Release:
        default:
            return "PhysXCommon.dll";
    }
}

#if X_PLATFORM_WIN32

FARPROC WINAPI delayHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
    switch (dliNotify) {
        case dliStartProcessing:
            break;

        case dliNotePreLoadLibrary: {
            X_PROFILE_NO_HISTORY_BEGIN("PhysicsLoadDll", core::profiler::SubSys::PHYSICS);

            const char* pName = gDelayLoadHook.getPhysxName(pdli->szDll);
            auto module = core::Module::Load(pName);
            if (!module)
            {
                X_ERROR("Phys", "Failed to load: \"%s\"", pName);
            }

            return static_cast<FARPROC>(module);
        } break;

        case dliNotePreGetProcAddress:
            break;
        case dliNoteEndProcessing:
            break;

        default:
            return nullptr;
    }

    return nullptr;
}

FARPROC WINAPI delayLoadFail(uint32_t dliNotify, PDelayLoadInfo pdli)
{
    switch (dliNotify)
    {
        case dliFailLoadLib: {
            const char* pName = gDelayLoadHook.getPhysxName(pdli->szDll);
            X_FATAL("Phys", "Failed to load lib: \"%s\"", pName);
            break;
        }
        case dliFailGetProc: {
            core::StackString256 func;

            auto& dlp = pdli->dlp;
            if (dlp.fImportByName) {
                func.set(dlp.szProcName);
            }
            else {
                func.setFmt("<0x%" PRIx32 ">", dlp.dwOrdinal);
            }

            X_FATAL("Phys", "Failed to resolved: \"%s\"", func.c_str());
            break;
        }
        default: 
            break;
    }

    return nullptr;
}

// extern c it so it don't have namespace name.

extern "C"

#if _MSC_FULL_VER >= 190024210 && !defined(DELAYIMP_INSECURE_WRITABLE_HOOKS)
    const
#endif // !_MSC_FULL_VER

    PfnDliHook __pfnDliNotifyHook2
    = delayHook;

#endif //!X_PLATFORM_WIN32

extern "C"

#if _MSC_FULL_VER >= 190024210 && !defined(DELAYIMP_INSECURE_WRITABLE_HOOKS)
const
#endif // !_MSC_FULL_VER

    PfnDliHook __pfnDliFailureHook2
    = delayLoadFail;

X_NAMESPACE_END