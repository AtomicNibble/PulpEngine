#include "stdafx.h"

#if 0

int const EpochStart = INT_MIN;

extern "C" ULONG _tls_index = 0;

extern "C"
{
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

BOOL WINAPI _DllMainCRTStartup(
	HINSTANCE hInstDll,         // handle to the DLL module
	DWORD fdwReason,            // reason for calling function
	LPVOID lpvReserved         // reserved
	)
{
	X_UNUSED(hInstDll);
	X_UNUSED(fdwReason);
	X_UNUSED(lpvReserved);

	return TRUE;
}

#endif