#include "EngineCommon.h"

#include "Cpu.h"
#include <Util\BitUtil.h>

#if X_COMPILER_CLANG
#include <cpuid.h>
#else
#include <intrin.h>
#endif // !X_COMPILER_CLANG

X_NAMESPACE_BEGIN(core)

namespace
{
    void cpuid(void* pCPUInfo, int infoType)
    {
#if X_COMPILER_CLANG
        int* pInts = reinterpret_cast<int*>(pCPUInfo);
        __cpuid(infoType, pInts[0], pInts[1], pInts[2], pInts[3]);
#else
#ifdef _WIN64
        __cpuid(reinterpret_cast<int*>(pCPUInfo), infoType);
#else
        __asm
        {
			mov    esi, pCPUInfo
			mov    eax, infoType
			xor    ecx, ecx
			cpuid
			mov    dword ptr[esi], eax
			mov    dword ptr[esi + 4], ebx
			mov    dword ptr[esi + 8], ecx
			mov    dword ptr[esi + 0Ch], edx
        }
#endif
#endif // !X_COMPILER_CLANG
    }

    DWORD NumExtended()
    {
#ifdef _WIN64
        return 0;
#else
        __asm
        {
			mov    eax, 0x80000000
			cpuid
			xor	   eax, 0x80000000
        }
#endif
    }

    void ProcessVendor(CpuInfo::CpuID::Info0& Info, char* Name)
    {
        // GO !
        cpuid(&Info, 0);

#if _WIN64
        *((int*)Name) = Info.ebx.name_;
        *((int*)(Name + 4)) = Info.edx.name_;
        *((int*)(Name + 8)) = Info.ecx.name_;
#else
        __asm {
			mov esi, Name
				mov edi, Info // 4 ints

				mov         ecx, dword ptr[edi + 4]
				mov         dword ptr[esi], ecx

				mov         edx, dword ptr[edi + 0Ch]
				mov         dword ptr[esi + 4], edx

				mov         eax, dword ptr[edi + 8]
				mov         dword ptr[esi + 8], eax
        }
#endif
    }

    void ProcessCPUName(char* Name)
    {
#ifdef _WIN64
        int NameInfo[4] = {0};

        cpuid(NameInfo, 0x80000002);
        memcpy(Name, NameInfo, sizeof(NameInfo));

        cpuid(NameInfo, 0x80000003);
        memcpy(Name + 16, NameInfo, sizeof(NameInfo));

        cpuid(NameInfo, 0x80000004);
        memcpy(Name + 32, NameInfo, sizeof(NameInfo));
#else
        __asm {
			mov esi, Name

				mov     eax, 0x80000002
				cpuid
				mov     DWORD PTR[esi + 0], eax
				mov     DWORD PTR[esi + 4], ebx
				mov     DWORD PTR[esi + 8], ecx
				mov     DWORD PTR[esi + 12], edx

				mov     eax, 0x80000003
				cpuid
				mov     DWORD PTR[esi + 16], eax
				mov     DWORD PTR[esi + 20], ebx
				mov     DWORD PTR[esi + 24], ecx
				mov     DWORD PTR[esi + 28], edx

				mov     eax, 0x80000004
				cpuid
				mov     DWORD PTR[esi + 32], eax
				mov     DWORD PTR[esi + 36], ebx
				mov     DWORD PTR[esi + 40], ecx
				mov     DWORD PTR[esi + 44], edx
        }
#endif
    }

    bool HasCPUID(void)
    {
#if defined(_WIN64) || 1
        return true;
#else
        __asm {
			pushfd // save eflags
				pop		eax
				test	eax, 0x00200000 // check ID bit
				jz		set21 // bit 21 is not set, so jump to set_21
				and		eax, 0xffdfffff // clear bit 21
				push	eax // save new value in register
				popfd // store new value in flags
				pushfd
				pop		eax
				test	eax, 0x00200000 // check ID bit
				jz		good
				jmp		err // cpuid not supported
			set21 :
			or		eax, 0x00200000 // set ID bit
				push	eax // store new value
				popfd // store new value in EFLAGS
				pushfd
				pop		eax
				test	eax, 0x00200000 // if bit 21 is on
				jnz		good
				jmp		err
        }
    err:
        return false;
    good:
        return true;
#endif
    }
} // namespace

// Loads all the info.
CpuInfo::CpuInfo(void)
{
    _SYSTEM_LOGICAL_PROCESSOR_INFORMATION cpuInfo[64];

    DWORD Len = sizeof(cpuInfo);

    zero_object(cpuInfo);
    zero_object(*this);

    if (!HasCPUID()) {
        X_WARNING("CpuInfo", "CPU dose not support cpuid");
        return;
    }

    ProcessVendor(info0_, cpuVendor_);

    if (info0_.eax.maxValidValue_ >= 1) {
        cpuid(&info1_, 1);
        cpuid(&infoEx0_, 0x80000000);

        if (infoEx0_.eax.maxValidValue_ >= 0x80000001) {
            cpuid(&infoEx1_, 0x80000001);
        }

        if (infoEx0_.eax.maxValidValue_ >= 0x80000004) {
            ProcessCPUName(cpuName_);
        }
    }

    // Get Chace INFO
    if (GetLogicalProcessorInformation(cpuInfo, &Len)) {
        int num = safe_static_cast<int, DWORD>(Len / sizeof(_SYSTEM_LOGICAL_PROCESSOR_INFORMATION));

        for (int32_t i = 0; i < num; i++) {
            _LOGICAL_PROCESSOR_RELATIONSHIP& Rel = cpuInfo[i].Relationship;

            switch (Rel) {
                case RelationProcessorCore:
                    coreCount_++;
                    logicalProcessorCount_ += bitUtil::CountBits(cpuInfo[i].ProcessorMask);
                    break;

                case RelationNumaNode:
                case RelationProcessorPackage:
                case RelationGroup:
                case RelationAll:
                    break;

                case RelationCache: {
                    CACHE_DESCRIPTOR& Cache = cpuInfo[i].Cache;

                    if (Cache.Level <= 3) {
                        uint32_t& Count = cacheCount_[Cache.Level - 1];

                        if (Count < 0x40) {
                            CacheInfo& Info = caches_[Cache.Level - 1][Count++];

                            Info.associativity_ = Cache.Associativity;
                            Info.lineSize_ = Cache.LineSize;
                            Info.size_ = Cache.Size;
                            Info.type_ = Cache.Type;
                        }
                        else {
                            Count++; // inc before assert
                            X_ASSERT(false, "Unexpected number of caches at level %d.", Cache.Level)
                            (Count);
                        }
                    }
                    else {
                        X_ASSERT(false, "Unexpected cache level.")
                        (Cache.Level);
                    }

                } break;
            }
        }
    }
}

X_NAMESPACE_END