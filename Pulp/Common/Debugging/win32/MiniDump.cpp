#include "EngineCommon.h"

#include "MiniDump.h"

#include <DbgHelp.h>

#include "Util\LastError.h"

X_NAMESPACE_BEGIN(core)


namespace debugging
{
	bool WriteMiniDump(const char* filename, EXCEPTION_POINTERS* exceptionPointers)
	{
#if X_ENABLE_MINI_DUMP
		MINIDUMP_EXCEPTION_INFORMATION miniDumpInfo;

		MINIDUMP_TYPE DumpType = MiniDumpNormal;
		PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam = nullptr;
		PMINIDUMP_CALLBACK_INFORMATION CallbackParam = nullptr;

		miniDumpInfo.ThreadId = GetCurrentThreadId();
		miniDumpInfo.ExceptionPointers = exceptionPointers;
		miniDumpInfo.ClientPointers = FALSE;

		HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

		bool res = false;

		if ( hFile == INVALID_HANDLE_VALUE )
		{
			core::lastError::Description err;
			X_ERROR("MiniDump", "Cannot obtain handle for file \"%s\". Error: %s", filename, lastError::ToString( err ) );
		}
		else
		{
			BOOL success = MiniDumpWriteDump( 
				GetCurrentProcess(), 
				GetCurrentProcessId(),
				hFile,
				DumpType,
				&miniDumpInfo,
				UserStreamParam,
				CallbackParam
			);

			if( !success )
			{
				// shieeeeeeet
				core::lastError::Description err;
				X_ERROR("MiniDump", "Cannot write minidump to file \"%s\". Error: %s", filename, lastError::ToString( err ) );

			}

			res = success == TRUE;
			::CloseHandle(hFile);
		}
		return res;
#else
		X_UNUSED(filename);
		X_UNUSED(exceptionPointers);
		return true;
#endif
	}
}



X_NAMESPACE_END