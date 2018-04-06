#pragma once

#ifndef X_SYMBOLRESOLUTION_H_
#define X_SYMBOLRESOLUTION_H_

#include "Debugging/SymbolInfo.h"

X_NAMESPACE_BEGIN(core)

/// \ingroup Debugging
/// \brief Contains a facility for resolving debugging symbols for functions.
/// \details When dealing with low-level details like call stacks and stack frames, normally only function addresses
/// can be gathered at run-time. With the help of debugging symbols, those function addresses can be converted into
/// the filename and line number the function originated from. Resolving debugging symbols works for user code,
/// third-party code, and OS code, as long as the corresponding symbol files are available.
///
/// Most Windows symbol files can be manually downloaded in separate packages, or automatically downloaded by using
/// symbol servers. See http://msdn.microsoft.com/en-us/windows/hardware/gg463028.aspx for more details.
/// \remark "Linker -> Debugging -> Generate Debug Info" should be set to "Yes" in all builds in order to make
/// debugging symbols available via .pdb files.
/// \remark Code generation is enabled/disabled via the preprocessor option \ref X_ENABLE_SYMBOL_RESOLUTION. Disabling
/// symbol resolution reduces the executable's size, and may be useful for retail builds.
/// \sa X_ENABLE_SYMBOL_RESOLUTION SymbolInfo
namespace symbolResolution
{
    /// \brief Starts the symbol resolution mechanism.
    /// \remark This is called automatically when starting the Core module.
    void Startup(void);

    /// \brief Refreshes the modules
    /// \remark This must be called after startup.
    void Refresh(void);

    /// \brief Shuts down the symbol resolution mechanism.
    /// \remark This is called automatically when shutting down the Core module.
    void Shutdown(void);

    /// \brief Resolves the symbol information for a given address.
    /// \sa SymbolInfo
    SymbolInfo ResolveSymbolsForAddress(const void* const address);
} // namespace symbolResolution

X_NAMESPACE_END

#endif
