#pragma once

// In Win32 Release we use static linkage
#if defined(WIN32)
#if !defined(X_DLL)
#ifndef X_LIB
#define X_LIB
#endif
#else
#ifndef _USRDLL
#define _USRDLL
#endif
#endif
#endif // WIN32