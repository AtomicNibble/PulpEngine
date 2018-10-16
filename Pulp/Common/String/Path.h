#pragma once

#ifndef _X_STRING_PATH_H_
#define _X_STRING_PATH_H_

#include "StackString.h"

X_NAMESPACE_BEGIN(core)

template<typename TChar = char>
class Path : public StackString<MAX_PATH, TChar>
{
    typedef StackString<MAX_PATH, TChar> BaseType;

public:
    static const size_t BUF_SIZE = MAX_PATH;

public:
    static const TChar SLASH = '/';
    static const TChar INVALID_SLASH = '\\';

    static const wchar_t SLASH_W = L'/';
    static const wchar_t INVALID_SLASH_W = L'\\';

    inline Path();
    inline Path(const Path& oth);
    template<typename TCharOth>
    inline explicit Path(const Path<TCharOth>& oth);
    template<typename TCharOth>
    inline explicit Path(const TCharOth* const beginInclusive, const TCharOth* const endExclusive);

    inline explicit Path(const TChar* const str);

    Path(const TChar* const beginInclusive, const TChar* const endExclusive);

    using BaseType::set;
    template<typename TCharOth>
    inline void set(const TCharOth* const beginInclusive, const TCharOth* const endExclusive);

    using BaseType::append;
    template<typename TCharOth>
    inline void append(const TCharOth* const beginInclusive, const TCharOth* const endExclusive);

    inline const TChar* fileName(void) const;
    inline const TChar* extension(bool incDot = true) const;

    inline void setExtension(const TChar* pExtension);
    inline void setFileName(const TChar* pFilename);
    inline void setFileName(const TChar* pFileNameBegin, const TChar* pFileNameEnd);

    inline void operator=(const TChar*);

    inline const Path operator/(const Path<TChar>& oth) const;
    inline const Path operator/(const TChar*) const;

    inline const Path& operator/=(const Path<TChar>& oth);
    inline const Path& operator/=(const TChar*);

    inline const Path operator+(const Path<TChar>& oth) const;
    inline const Path operator+(const TChar*) const;

    inline const Path& operator+=(const Path<TChar>& oth);
    inline const Path& operator+=(const TChar*);

    inline void ensureSlash(void);
    inline void replaceSeprators(void);
    inline void removeFileName(void);
    inline void removeExtension(void);
    inline void removeTrailingSlash(void);
    // this is here so I can pass a path to win32 which requires 2 null terms.
    // so this makes it a little less hackey than memset the path instance 1st.
    inline size_t fillSpaceWithNullTerm(void);

    inline bool isAbsolute(void) const;

    inline int8_t getDriveNumber(void) const;
};

#include "Path.inl"

X_NAMESPACE_END

#endif // !_X_STRING_PATH_H_
