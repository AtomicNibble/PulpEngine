#pragma once

#if defined(WIN32)
#if !defined(X_DLL)
#ifndef TELEM_LIB
#define TELEM_LIB
#endif
#else
#ifndef _USRDLL
#define _USRDLL
#endif
#endif
#endif // WIN32