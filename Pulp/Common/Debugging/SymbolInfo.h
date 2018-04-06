#pragma once

#ifndef X_SYMBOLINFO_H
#define X_SYMBOLINFO_H

#include "String/StackString.h"
#include "String\Path.h"

X_NAMESPACE_BEGIN(core)

/// \ingroup Debugging
/// \class SymbolInfo
/// \brief Stores debugging symbol information.
/// \details This class stores information such as function name, file name, and line number for debugging symbols,
/// mostly used internally by the symbol resolution facility.
/// \sa symbolResolution
class SymbolInfo
{
public:
    /// \brief Constructs a symbol info instance, copying the given arguments.
    /// \remark Ownership of the provided arguments stays at the calling site.
    SymbolInfo(const char* const function, const char* const filename, unsigned int line);

    /// Returns the symbol's function name.
    X_INLINE const char* GetFunction(void) const;

    /// Returns the symbol's file name.
    X_INLINE const char* GetFilename(void) const;

    /// Returns the symbol's line number.
    X_INLINE unsigned int GetLine(void) const;

private:
    X_NO_ASSIGN(SymbolInfo);

    StackString512 function_;
    Path<char> filename_;
    uint32_t line_;
};

#include "SymbolInfo.inl"

X_NAMESPACE_END

#endif
