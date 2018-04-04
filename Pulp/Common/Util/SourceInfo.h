#pragma once

#ifndef X_SOURCEINFO_H_
#define X_SOURCEINFO_H_

X_NAMESPACE_BEGIN(core)

struct SourceInfo
{
    inline SourceInfo(const char* const module, const char* const file,
        int line, const char* const function,
        const char* const functionSignature);

    const char* const pModule_; // the module it came from.
    const char* const pFile_;
    const char* const pFunction_;
    const char* const pFunctionSignature_;
    const int line_;

private:
    X_NO_ASSIGN(SourceInfo);
};

#include "SourceInfo.inl"

X_NAMESPACE_END

#endif // X_SOURCEINFO_H_
