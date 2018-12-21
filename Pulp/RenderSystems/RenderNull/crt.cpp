#include "stdafx.h"

#if 1

X_DISABLE_EMPTY_FILE_WARNING

#else

int const EpochStart = INT_MIN;

typedef void(__cdecl *_PVFV)();
extern _PVFV __xc_a[], __xc_z[];    // C++ initializers

#pragma section(".CRT$XCA", read, write)
#pragma data_seg(".CRT$XCA") // start of ctor section
_PVFV __xc_a[] = { 0 };

#pragma section(".CRT$XCZ", read, write)
#pragma data_seg(".CRT$XCZ") // end of ctor section
_PVFV __xc_z[] = { 0 };


extern "C"
{
	void _initterm(_PVFV *pfbegin, _PVFV *pfend);

	ULONG _tls_index = 0;
	int _Init_global_epoch = EpochStart;

	__declspec(thread) int _Init_thread_epoch = EpochStart;
}

extern "C" void __cdecl _Init_thread_header(int* const pOnce)
{
	X_UNUSED(pOnce);

}

extern "C" void __cdecl _Init_thread_abort(int* const pOnce)
{
	X_UNUSED(pOnce);

}

extern "C" void __cdecl _Init_thread_footer(int* const pOnce)
{
	X_UNUSED(pOnce);

}

void* __cdecl operator new(size_t cb)
{
	return HeapAlloc(GetProcessHeap(), 0, cb);
}

void __cdecl operator delete(void* pv)
{
	if (pv) {
		HeapFree(::GetProcessHeap(), 0, pv);
	}
}

void __cdecl operator delete(void* pv, size_t size)
{
	X_UNUSED(size);
	if (pv) {
		HeapFree(::GetProcessHeap(), 0, pv);
	}
}


extern "C" int _cdecl _purecall(void)
{
	return 0;
}

int atexit(void(__cdecl *func)(void))
{
	X_UNUSED(func);
	return 0;
}


BOOL WINAPI _DllMainCRTStartup(HINSTANCE hInstDll, DWORD reason, LPVOID lpvReserved)
{
	X_UNUSED(hInstDll);
	X_UNUSED(lpvReserved);

	if (reason == DLL_PROCESS_ATTACH)
	{
		_initterm(__xc_a, __xc_z);
	}

	return TRUE;
}

extern "C"
{

	void _initterm(_PVFV *pfbegin, _PVFV *pfend)
	{
		while (pfbegin < pfend)
		{
			if (*pfbegin != nullptr) {
				(**pfbegin)();
			}
			++pfbegin;
		}
	}

}

#endif