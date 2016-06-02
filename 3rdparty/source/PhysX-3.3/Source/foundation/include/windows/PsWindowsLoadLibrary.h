/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef PS_WINDOWS_LOADLIBRARY_H
#define PS_WINDOWS_LOADLIBRARY_H

#include "foundation/PxPreprocessor.h"
#include "common/windows/PxWindowsDelayLoadHook.h"
#include "windows/PsWindowsInclude.h"

#ifdef PX_SECURE_LOAD_LIBRARY
#include "nvSecureLoadLibrary.h"
#endif


namespace physx
{
namespace shdfnd
{
	EXTERN_C IMAGE_DOS_HEADER __ImageBase;

	// PhysXCommon x64 dll name 
	static const char* physXCommonDebugx64 = "PhysXCommonDEBUG.dll";
	static const char* physXCommonCheckedx64 = "PhysXCommonCHECKED.dll";
	static const char* physXCommonProfilex64 = "PhysXCommonPROFILE.dll";
	static const char* physXCommonReleasex64 = "PhysXCommon.dll";

	// PhysXCommon x86 dll name 
	static const char* physXCommonDebugx86 = "PhysXCommonDEBUG.dll";
	static const char* physXCommonCheckedx86= "PhysXCommonCHECKED.dll";
	static const char* physXCommonProfilex86 = "PhysXCommonPROFILE.dll";
	static const char* physXCommonReleasex86 = "PhysXCommon.dll";


	PX_INLINE HMODULE WINAPI loadLibrary(const char* name)
	{
#ifdef PX_WINMODERN
		// convert to unicode
		const static int BUFFERSIZE = 256;
		WCHAR buffer[BUFFERSIZE];
		int succ = MultiByteToWideChar(CP_ACP, 0, name, -1, buffer, BUFFERSIZE);

		// validate
		if (succ < 0)
			succ = 0;
		if (succ < BUFFERSIZE)
			buffer[succ] = 0;
		else if (buffer[BUFFERSIZE-1])
			buffer[0] = 0;

		return (succ > 0) ? ::LoadPackagedLibrary(buffer, 0) : NULL;
#else
#ifdef PX_SECURE_LOAD_LIBRARY
		HMODULE retVal = nvLoadSignedLibrary(name,true);
		if(!retVal)
		{
			exit(1);
		}
		return retVal;
#else
		return ::LoadLibraryA( name );
#endif
#endif		
	};

	PX_INLINE FARPROC WINAPI physXCommonDliNotePreLoadLibrary(const char* libraryName, const physx::PxDelayLoadHook* delayLoadHook)
	{	
		if(!delayLoadHook)
		{
			const char* libraryToLoad = libraryName;
#ifndef PX_WINMODERN
			char buffer[MAX_PATH] = {0};
			GetModuleFileName((HINSTANCE)&__ImageBase, buffer, MAX_PATH);
			
			if(strstr(buffer,"_x64.dll"))
			{
				libraryToLoad = physXCommonReleasex64;
				if(strstr(buffer,"DEBUG_x64.dll"))
				{
					libraryToLoad = physXCommonDebugx64;
				}
				if(strstr(buffer,"CHECKED_x64.dll"))
				{
					libraryToLoad = physXCommonCheckedx64;
				}
				if(strstr(buffer,"PROFILE_x64.dll"))
				{
					libraryToLoad = physXCommonProfilex64;
				}
			}

			if(strstr(buffer,"_x86.dll"))
			{
				libraryToLoad = physXCommonReleasex86;
				if(strstr(buffer,"DEBUG_x86.dll"))
				{
					libraryToLoad = physXCommonDebugx86;
				}
				if(strstr(buffer,"CHECKED_x86.dll"))
				{
					libraryToLoad = physXCommonCheckedx86;
				}
				if(strstr(buffer,"PROFILE_x86.dll"))
				{
					libraryToLoad = physXCommonProfilex86;
				}
			}
#endif
			return (FARPROC)shdfnd::loadLibrary(libraryToLoad);
		}
		else
		{
			if( strcmp( libraryName, shdfnd::physXCommonDebugx64 ) == 0 )
			{
				return (FARPROC)shdfnd::loadLibrary(delayLoadHook->getPhysXCommonDEBUGDllName());
			}

			if( strcmp( libraryName, shdfnd::physXCommonCheckedx64 ) == 0 )
			{
				return (FARPROC)shdfnd::loadLibrary( delayLoadHook->getPhysXCommonCHECKEDDllName() );
			}

			if( strcmp( libraryName, shdfnd::physXCommonProfilex64 ) == 0 )
			{
				return  (FARPROC)shdfnd::loadLibrary( delayLoadHook->getPhysXCommonPROFILEDllName() );
			}

			if( strcmp( libraryName, shdfnd::physXCommonReleasex64 ) == 0 )
			{
				return  (FARPROC)shdfnd::loadLibrary( delayLoadHook->getPhysXCommonDllName() );
			}

		}
		return NULL;
	}

} // namespace shdfnd
} // namespace physx


#endif	// PS_WINDOWS_LOADLIBRARY_H
