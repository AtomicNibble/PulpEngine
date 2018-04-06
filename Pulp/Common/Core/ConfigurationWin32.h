#pragma once
#ifndef X_CORECONFIGURATIONWIN32_H
#define X_CORECONFIGURATIONWIN32_H

#if X_SUPER
// the X_ASSERT_NOT_NULL macro triggers this warning in master builds
#pragma warning(disable : 4555)
#endif

// project settings should be set to /WAll, which turns on ALL warnings, even ones which are off by default at warning level 4.
// however, some of these warnings will surely fire in ANY code, hence we turn them off.
#pragma warning(disable : 4514) // 'function' : unreferenced inline function has been removed --- happens with every template function not used
#pragma warning(disable : 4820) // 'bytes' bytes padding added after construct 'member_name' --- most structs and classes will have some padding
#pragma warning(disable : 4275) // non dll-interface class 'stdext::exception' used as base for dll-interface class 'std::bad_cast'

#pragma warning(disable : 4350) // stupid string shit O.o

// these warnings are informational, and might be useful during development to see what was inlined and what not.
// however, we turn them off by default because a lot of them are generated.
#pragma warning(disable : 4710) // 'function' : function not inlined
#pragma warning(disable : 4711) // function 'function' selected for inline expansion

// we use nonstandard extensions for the "override", "abstract" and "sealed" keyword.
// it is impossible to compile Windows code without using nonstandard extensions, thus we put them to good use.
#pragma warning(disable : 4481) // nonstandard extension used: override specifier 'specifier'

#if X_DEBUG
#define _HAS_EXCEPTIONS 0
#define _HAS_ITERATOR_DEBUGGING 1
#define _SECURE_SCL 1
#elif X_RELEASE
#define _HAS_EXCEPTIONS 0
#define _HAS_ITERATOR_DEBUGGING 0
#define _SECURE_SCL 0
#elif X_SUPER
#define _HAS_EXCEPTIONS 0
#define _HAS_ITERATOR_DEBUGGING 0
#define _SECURE_SCL 0
#endif

#endif
