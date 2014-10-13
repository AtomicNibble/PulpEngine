#pragma once


#ifndef _X_STRING_PATH_H_
#define _X_STRING_PATH_H_

#include "StackString.h"

X_NAMESPACE_BEGIN(core)


class Path : public StackString<MAX_PATH>
{
public:
#ifdef X_PLATFORM_WIN32
	static const char NATIVE_SLASH = '\\';
	static const char NON_NATIVE_SLASH = '/';

	// using none native slash on windows dose not work well
	// since the drive slash must be a native one.
//	static const char NATIVE_SLASH = '/';
//	static const char NON_NATIVE_SLASH = '\\';
#else
	static const char NATIVE_SLASH = '/';
	static const char NON_NATIVE_SLASH = '\\';
#endif

	inline Path();
	inline Path(const Path& oth);
	inline explicit Path(const char* const str);


	inline const char* fileName(void) const;
	inline const char* extension(void) const;

	inline void setExtension(const char* extension);
	inline void setFileName(const char* filename);

	
	inline void operator=(const char*);

	inline const Path operator/(const Path& oth) const;
	inline const Path operator/(const char*) const;

	inline const Path& operator/=(const Path& oth);
	inline const Path& operator/=(const char*);

	inline void ensureSlash(void);
	inline void replaceSeprators(void);
	inline void removeFileName(void);
	inline void removeExtension(void);

	inline bool isAbsolute(void) const;

};


#include "Path.inl"

X_NAMESPACE_END

#endif // !_X_STRING_PATH_H_
