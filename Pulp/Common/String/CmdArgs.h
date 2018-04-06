#pragma once

#ifndef _X_CMDARGS_X_H_
#define _X_CMDARGS_X_H_

#include "Lexer.h"

X_NAMESPACE_BEGIN(core)

template<size_t BUF_SIZE, typename TChar = char>
class CmdArgs
{
    static const size_t MAX_COMMAND_ARGS = 64;

public:
    X_INLINE CmdArgs(void);
    X_INLINE explicit CmdArgs(const TChar* pText);

    X_INLINE void clear(void);

    X_INLINE size_t getArgc(void) const;
    X_INLINE const TChar* getArgv(size_t arg) const;

    // returns the value that follows a option.
    X_INLINE const TChar* getOption(const TChar* pOptionName) const;
    // check if a flag is present.
    X_INLINE bool hasFlag(const TChar* pFlag) const;

    X_INLINE void AppendArg(const TChar* pArg);

private:
    X_INLINE void tokenize(const TChar* pText);

private:
    size_t argc_;
    TChar* argv_[MAX_COMMAND_ARGS];
    TChar tokenized_[BUF_SIZE];
};

template<size_t BUF_SIZE>
class CmdArgs<BUF_SIZE, char>
{
    static const size_t MAX_COMMAND_ARGS = 64;

public:
    X_INLINE CmdArgs(void);
    X_INLINE explicit CmdArgs(const char* pText);

    X_INLINE void clear(void);

    X_INLINE size_t getArgc(void) const;
    X_INLINE const char* getArgv(size_t arg) const;

    // returns the value that follows a option.
    X_INLINE const char* getOption(const char* pOptionName) const;
    // check if a flag is present.
    X_INLINE bool hasFlag(const char* pFlag) const;

    X_INLINE void AppendArg(const char* pArg);

private:
    X_INLINE void tokenize(const char* pText);

private:
    size_t argc_;
    char* argv_[MAX_COMMAND_ARGS];
    char tokenized_[BUF_SIZE];
};

template<size_t BUF_SIZE>
class CmdArgs<BUF_SIZE, wchar_t>
{
    static const size_t MAX_COMMAND_ARGS = 64;

public:
    X_INLINE CmdArgs(void);
    X_INLINE explicit CmdArgs(const wchar_t* pText);

    X_INLINE void clear(void);

    X_INLINE size_t getArgc(void) const;
    X_INLINE const wchar_t* getArgv(size_t arg) const;

    // returns the value that follows a option.
    X_INLINE const wchar_t* getOption(const wchar_t* pOptionName) const;
    // check if a flag is present.
    X_INLINE bool hasFlag(const wchar_t* pFlag) const;

    X_INLINE void AppendArg(const wchar_t* pArg);

private:
    X_INLINE void tokenize(const wchar_t* pText);

private:
    size_t argc_;
    wchar_t* argv_[MAX_COMMAND_ARGS];
    wchar_t tokenized_[BUF_SIZE];
};

X_NAMESPACE_END

#include "CmdArgs.inl"

#endif // _X_CMDARGS_X_H_