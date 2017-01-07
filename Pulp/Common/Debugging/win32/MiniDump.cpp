#include "EngineCommon.h"

#include "MiniDump.h"

X_DISABLE_WARNING(4091)
#include <DbgHelp.h>
X_ENABLE_WARNING(4091)

#include "Util\LastError.h"

X_NAMESPACE_BEGIN(core)


namespace debugging
{
	namespace
	{
		uint32_t getDumpType(DumpType type)
		{
			switch (type)
			{
				case DumpType::Full:
					return MiniDumpWithDataSegs | 
						MiniDumpWithFullMemoryInfo |
						MiniDumpWithThreadInfo |
						MiniDumpWithProcessThreadData |
						MiniDumpWithUnloadedModules |
						MiniDumpWithPrivateReadWriteMemory;

				case DumpType::Medium:
					return MiniDumpWithDataSegs |
						MiniDumpWithFullMemory |
						MiniDumpWithFullMemoryInfo |
						MiniDumpWithThreadInfo |
						MiniDumpWithProcessThreadData |
						MiniDumpWithUnloadedModules |
						MiniDumpWithPrivateReadWriteMemory;

				case DumpType::Small:
					return MiniDumpNormal;
			}
	
			return MiniDumpNormal;
		}

	} // namespace

	bool WriteMiniDump(const Path<char>& filename, DumpType type, EXCEPTION_POINTERS* exceptionPointers)
	{
#if X_ENABLE_MINI_DUMP
		MINIDUMP_EXCEPTION_INFORMATION miniDumpInfo;

		MINIDUMP_TYPE DumpType = static_cast<MINIDUMP_TYPE>(getDumpType(type));
		PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam = nullptr;
		PMINIDUMP_CALLBACK_INFORMATION CallbackParam = nullptr;

		miniDumpInfo.ThreadId = core::Thread::GetCurrentID();
		miniDumpInfo.ExceptionPointers = exceptionPointers;
		miniDumpInfo.ClientPointers = FALSE;

		HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

		bool res = false;

		if ( hFile == INVALID_HANDLE_VALUE )
		{
			core::lastError::Description err;
			X_ERROR("MiniDump", "Cannot obtain handle for file \"%s\". Error: %s", filename.c_str(), lastError::ToString( err ) );
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
				X_ERROR("MiniDump", "Cannot write minidump to file \"%s\". Error: %s", filename.c_str(), lastError::ToString( err ) );

			}

			res = success == TRUE;
			::CloseHandle(hFile);
		}
		return res;
#else
		X_UNUSED(filename);
		X_UNUSED(type);
		X_UNUSED(exceptionPointers);
		return true;
#endif
	}
}



X_NAMESPACE_END