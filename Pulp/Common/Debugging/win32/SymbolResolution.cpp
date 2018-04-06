#include "EngineCommon.h"
#include "SymbolResolution.h"
#include <Util\LastError.h>

X_DISABLE_WARNING(4091)
#include <DbgHelp.h>
X_ENABLE_WARNING(4091)

X_NAMESPACE_BEGIN(core)

#if X_ENABLE_SYMBOL_RESOLUTION

#endif

namespace symbolResolution
{
    namespace
    {
        const char* UserSearchPath = NULL;

        struct XSymbolInfoPackage : public SYMBOL_INFO_PACKAGE
        {
            XSymbolInfoPackage()
            {
                si.SizeOfStruct = sizeof(SYMBOL_INFO);
                si.MaxNameLen = sizeof(name);
            }
        };

        struct XImgHelpLine : public _IMAGEHLP_LINE64
        {
            XImgHelpLine()
            {
                zero_this(this);
                this->SizeOfStruct = 24;
            }
        };

    } // namespace

    void Startup(void)
    {
#if X_ENABLE_SYMBOL_RESOLUTION
        //	X_LOG0( "SymbolResolution", "Loading debugging symbols." );

        SymSetOptions(SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);

        if (!SymInitialize(GetCurrentProcess(), UserSearchPath, TRUE)) {
            core::lastError::Description Dsc;
            X_ERROR("Symbols", "Symbols for this process could not be loaded. Error: %s", core::lastError::ToString(Dsc));
        }
#endif
    }

    void Refresh(void)
    {
#if X_ENABLE_SYMBOL_RESOLUTION

        if (!SymRefreshModuleList(GetCurrentProcess())) {
            core::lastError::Description Dsc;
            X_ERROR("Symbols", "Failed to refresh modules for this process. Error: %s", core::lastError::ToString(Dsc));
        }
#endif
    }

    void Shutdown(void)
    {
#if X_ENABLE_SYMBOL_RESOLUTION
        //	X_LOG0( "SymbolResolution", "Unloading debugging symbols." );

        if (!SymCleanup(GetCurrentProcess())) {
            X_ERROR("Symbols", "Symbols for this process could not be unloaded.");
        }
#endif
    }

    SymbolInfo ResolveSymbolsForAddress(const void* const address)
    {
#if X_ENABLE_SYMBOL_RESOLUTION
        DWORD64 displacement64 = 0;
        DWORD displacement = 0;

        XSymbolInfoPackage sip;
        XImgHelpLine line;

        HANDLE hProcess = GetCurrentProcess();

        if (!SymFromAddr(hProcess, reinterpret_cast<DWORD64>(address), &displacement64, &sip.si)) {
            // refresh and try again.
            core::symbolResolution::Refresh();
            if (!SymFromAddr(hProcess, reinterpret_cast<DWORD64>(address), &displacement64, &sip.si)) {
                core::lastError::Description Dsc;
                X_ERROR("Symbols", "SymFromAddr failed. Err: %s", core::lastError::ToString(Dsc));
                return SymbolInfo("function", "filename", 0);
            }
        }
        if (!SymGetLineFromAddr64(hProcess, reinterpret_cast<DWORD64>(address), &displacement, &line)) {
            core::lastError::Description Dsc;
            X_ERROR("Symbols", "SymGetLineFromAddr64 failed. Err: %s", core::lastError::ToString(Dsc));
            return SymbolInfo(sip.si.Name, "filename", 0);
        }

        return SymbolInfo(sip.si.Name, line.FileName, line.LineNumber);

#else
        X_UNUSED(address);
        return SymbolInfo("function", "filename", 0);
#endif
    }
} // namespace symbolResolution

X_NAMESPACE_END