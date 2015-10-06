#pragma once


#ifndef _X_STRING_PATH_H_
#define _X_STRING_PATH_H_

#include "StackString.h"

X_NAMESPACE_BEGIN(core)

template<typename TChar = char>
class Path : public StackString<MAX_PATH, TChar>
{
public:
#ifdef X_PLATFORM_WIN32
	static const TChar NATIVE_SLASH = '\\';
	static const TChar NON_NATIVE_SLASH = '/';

	static const wchar_t NATIVE_SLASH_W = L'\\';
	static const wchar_t NON_NATIVE_SLASH_W = L'/';

	// using none native slash on windows dose not work well
	// since the drive slash must be a native one.
//	static const TChar NATIVE_SLASH = '/';
//	static const TChar NON_NATIVE_SLASH = '\\';
#else
	static const TChar NATIVE_SLASH = '/';
	static const TChar NON_NATIVE_SLASH = '\\';
	static const wchar_t NATIVE_SLASH_W = L'/';
	static const wchar_t NON_NATIVE_SLASH_W = L'\\';
#endif

	inline Path();
	inline Path(const Path& oth);
	inline explicit Path(const TChar* const str);


	inline const TChar* fileName(void) const;
	inline const TChar* extension(void) const;

	inline void setExtension(const TChar* extension);
	inline void setFileName(const TChar* filename);

	
	inline void operator=(const TChar*);

	inline const Path operator/(const Path<TChar>& oth) const;
	inline const Path operator/(const TChar*) const;

	inline const Path& operator/=(const Path<TChar>& oth);
	inline const Path& operator/=(const TChar*);

	inline void ensureSlash(void);
	inline void replaceSeprators(void);
	inline void removeFileName(void);
	inline void removeExtension(void);
	inline void removeTrailingSlash(void);

	inline bool isAbsolute(void) const;

};


#include "Path.inl"

X_NAMESPACE_END

#endif // !_X_STRING_PATH_H_
