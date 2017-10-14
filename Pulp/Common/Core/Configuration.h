#pragma once

#ifndef _X_ENGINE_CONFIG_H_
#define _X_ENGINE_CONFIG_H_


#include "Prepro\PreproString.h"


// make sure the config is valid, and only one is set.
#if X_DEBUG 
#	if X_RELEASE
#		error Build configuration mismatch.
#	elif X_SUPER
#		error Build configuration mismatch.
#	endif
#elif X_RELEASE
#	if X_DEBUG
#		error Build configuration mismatch.
#	elif X_SUPER
#		error Build configuration mismatch.
#	endif
#elif X_SUPER
#	if X_DEBUG
#		error Build configuration mismatch.
#	elif X_RELEASE
#		error Build configuration mismatch.
#	endif
#else
#	error Unknown build configuration.
#endif



#if _WIN32
	#if X_64 == 1
		#define X_CPUSTRING						"x64"
	#else
		#define X_CPUSTRING						"x86"
	#endif // !X_64

	#define X_PLATFORM							win32
	#define X_PLATFORM_STR						"win32"
	#define X_PLATFORM_WIN32					1
	#include "ConfigurationWin32.h"

#else // !_WIN32
	#error Unknown platform.
#endif

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

#if defined(__clang__)
	#define X_COMPILER clang
	#define X_COMPILER_CLANG 1
	#define X_COMPILER_MSVC 0
#elif defined(_MSC_VER)
	#define X_COMPILER msvc
	#define X_COMPILER_MSVC 1
	#define X_COMPILER_CLANG 0
#else
	#error Unknown compiler.
#endif


#define X_ENGINE_NAME			"Potato"
#define X_ENGINE_VERSION		0.1
#define X_ENGINE_VERSION_STR	X_STRINGIZE(X_ENGINE_VERSION)
#define	X_BUILD_STRING			X_PLATFORM_STR "-" X_CPUSTRING
#define X_ENGINE_BUILD_REF		-1 // branch-ref

#define X_INCLUDE(path)			X_STRINGIZE(path)



#if X_DEBUG
#   define X_BUILD_TYPE "-debug" 
#	define X_ENABLE_SAFE_STATIC_CAST 1
#	define X_ENABLE_DEBUGGER_BREAKPOINTS 1
#	define X_ENABLE_STACK_ALLOCATOR_CHECK 1
#	define X_ENABLE_POOL_ALLOCATOR_CHECK 1
#	define X_ENABLE_MEMORY_ALLOCATOR_STATISTICS 1
#	define X_ENABLE_MEMORY_ALLOCATOR_DMALLOC_STATISTICS 0 // this is super slow.
#	define X_ENABLE_MEMORY_ARENA_STATISTICS 1
#	define X_ENABLE_MEMORY_DEBUG_POLICIES 1
#	define X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS 1
#	define X_ENABLE_MEMORY_SIMPLE_TRACKING 1
#	define X_ENABLE_MEMORY_SOURCE_INFO 1
#	define X_ENABLE_MEMORY_HUMAN_IDS 0
#	define X_ENABLE_MINI_DUMP 1
#	define X_ENABLE_SYMBOL_RESOLUTION 1
#	define X_ENABLE_UNHANDLED_EXCEPTION_HANDLER 1
#	define X_ENABLE_ASSERTIONS 1
#	define X_ENABLE_LOGGING 1
#	define X_ENABLE_LOGGING_SOURCE_INFO 1
#   define X_ENABLE_FILE_STATS 1
#   define X_ENABLE_FILE_ARTIFICAIL_DELAY 1
#	define X_ENABLE_DIR_WATCHER_LOGGING 1
#	define X_ENABLE_PROFILER 1
#	define X_ENABLE_PROFILER_WARNINGS 1
#	define X_ENABLE_JOBSYS_PARENT_CHECK 1
#	define X_ENABLE_JOBSYS_RECORD_SUBSYSTEM 1
#	define X_ENABLE_JOBSYS_PROFILER 1
#	define X_ENABLE_CONFIG_HOT_RELOAD 1
#elif X_RELEASE
#   define X_BUILD_TYPE "-release" 
#	define X_ENABLE_SAFE_STATIC_CAST 1
#	define X_ENABLE_DEBUGGER_BREAKPOINTS 1
#	define X_ENABLE_STACK_ALLOCATOR_CHECK 1
#	define X_ENABLE_POOL_ALLOCATOR_CHECK 1
#	define X_ENABLE_MEMORY_ALLOCATOR_STATISTICS 1
#	define X_ENABLE_MEMORY_ALLOCATOR_DMALLOC_STATISTICS 0 // this is super slow.
#	define X_ENABLE_MEMORY_ARENA_STATISTICS 1
#	define X_ENABLE_MEMORY_DEBUG_POLICIES 1
#	define X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS 1
#	define X_ENABLE_MEMORY_SIMPLE_TRACKING 1
#	define X_ENABLE_MEMORY_SOURCE_INFO 1
#	define X_ENABLE_MEMORY_HUMAN_IDS 0
#	define X_ENABLE_MINI_DUMP 1
#	define X_ENABLE_SYMBOL_RESOLUTION 0
#	define X_ENABLE_UNHANDLED_EXCEPTION_HANDLER 1
#	define X_ENABLE_ASSERTIONS 1
#	define X_ENABLE_LOGGING 1
#	define X_ENABLE_LOGGING_SOURCE_INFO 0
#   define X_ENABLE_FILE_STATS 1
#   define X_ENABLE_FILE_ARTIFICAIL_DELAY 1
#	define X_ENABLE_DIR_WATCHER_LOGGING 0
#	define X_ENABLE_PROFILER 1
#	define X_ENABLE_PROFILER_WARNINGS 1
#	define X_ENABLE_JOBSYS_PARENT_CHECK 0
#	define X_ENABLE_JOBSYS_RECORD_SUBSYSTEM 1
#	define X_ENABLE_JOBSYS_PROFILER 1
#	define X_ENABLE_CONFIG_HOT_RELOAD 1
#elif X_SUPER
#   define X_BUILD_TYPE "-ship" 
#	define X_ENABLE_SAFE_STATIC_CAST 0
#	define X_ENABLE_DEBUGGER_BREAKPOINTS 0
#	define X_ENABLE_STACK_ALLOCATOR_CHECK 0
#	define X_ENABLE_POOL_ALLOCATOR_CHECK 0
#	define X_ENABLE_MEMORY_ALLOCATOR_STATISTICS 0
#	define X_ENABLE_MEMORY_ALLOCATOR_DMALLOC_STATISTICS 0
#	define X_ENABLE_MEMORY_ARENA_STATISTICS 0
#	define X_ENABLE_MEMORY_DEBUG_POLICIES 0		 // enables debug polices
#	define X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS 0 // enables the debug polic definitions, set to zero to ensure none are in use.
#	define X_ENABLE_MEMORY_SIMPLE_TRACKING 0	 // allow simple tracking even when debug polices disabled.
#	define X_ENABLE_MEMORY_SOURCE_INFO 0
#	define X_ENABLE_MEMORY_HUMAN_IDS 0
#	define X_ENABLE_MINI_DUMP 0
#	define X_ENABLE_SYMBOL_RESOLUTION 0
#	define X_ENABLE_UNHANDLED_EXCEPTION_HANDLER 0
#	define X_ENABLE_ASSERTIONS 0
#	define X_ENABLE_LOGGING 1
#	define X_ENABLE_LOGGING_SOURCE_INFO 0
#   define X_ENABLE_FILE_STATS 0
#   define X_ENABLE_FILE_ARTIFICAIL_DELAY 0
#	define X_ENABLE_DIR_WATCHER_LOGGING 0
#	define X_ENABLE_PROFILER 0
#	define X_ENABLE_PROFILER_WARNINGS 0
#	define X_ENABLE_JOBSYS_PARENT_CHECK 0
#	define X_ENABLE_JOBSYS_RECORD_SUBSYSTEM 0
#	define X_ENABLE_JOBSYS_PROFILER 0
#	define X_ENABLE_CONFIG_HOT_RELOAD 0
#else
#error "No Build Type Defined"
#endif

#if X_COMPILER_CLANG

#undef X_ENABLE_ASSERTIONS
#define X_ENABLE_ASSERTIONS 0

#endif // X_COMPILER_CLANG

// TODO: find better place to put.
#if X_ENABLE_MEMORY_HUMAN_IDS
#define X_MEM_HUMAN_IDS_CB(x) , x
#else
#define X_MEM_HUMAN_IDS_CB(x)
#endif // !X_ENABLE_MEMORY_HUMAN_IDS


#if X_ENABLE_LOGGING_SOURCE_INFO
#define X_SOURCE_INFO_LOG_CB(x) , x
#define X_SOURCE_INFO_LOG_CA(x) x,
#else
#define X_SOURCE_INFO_LOG_CB(x)
#define X_SOURCE_INFO_LOG_CA(x)
#endif // !X_ENABLE_LOGGING_SOURCE_INFO


#endif // _X_ENGINE_CONFIG_H_