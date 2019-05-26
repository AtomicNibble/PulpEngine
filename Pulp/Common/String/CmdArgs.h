#pragma once

#ifndef _X_CMDARGS_X_H_
#define _X_CMDARGS_X_H_

#include "Lexer.h"

X_NAMESPACE_BEGIN(core)

template<size_t BUF_SIZE>
class CmdArgs
{
    static const size_t MAX_COMMAND_ARGS = 64;
    using TChar = char;

public:
    X_INLINE CmdArgs(void);
    X_INLINE explicit CmdArgs(const TChar* pText);
    X_INLINE CmdArgs(const TChar* pText, size_t length);

    X_INLINE void clear(void);

    X_INLINE size_t getArgc(void) const;
    X_INLINE const TChar* getArgv(size_t arg) const;

    // returns the value that follows a option.
    X_INLINE const TChar* getOption(const TChar* pOptionName) const;
    // check if a flag is present.
    X_INLINE bool hasFlag(const TChar* pFlag) const;

    X_INLINE void appendArg(const TChar* pArg);

private:
    X_INLINE void tokenize(const TChar* pText, size_t length);

private:
    size_t argc_;
    TChar* argv_[MAX_COMMAND_ARGS];
    TChar tokenized_[BUF_SIZE];
};

X_NAMESPACE_END

#include "CmdArgs.inl"

#endif // _X_CMDARGS_X_H_
