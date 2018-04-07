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
    using CpuIDData = std::array<char, 16>;
   
    void cpuid(CpuIDData& data, int infoType)
    {
        int* pInts = reinterpret_cast<int*>(data.data());
#if X_COMPILER_CLANG
        __cpuid(infoType, pInts[0], pInts[1], pInts[2], pInts[3]);
#else
        __cpuid(pInts, infoType);
#endif // !X_COMPILER_CLANG
    }

    template<typename T>
    void cpuid(T* pCPUInfo, int infoType)
    {
        static_assert(sizeof(T) == 16, "Invalid size");

        int* pInts = reinterpret_cast<int*>(pCPUInfo);
#if X_COMPILER_CLANG
        __cpuid(infoType, pInts[0], pInts[1], pInts[2], pInts[3]);
#else
        __cpuid(pInts, infoType);
#endif // !X_COMPILER_CLANG
    }

    int32_t NumExtended(void)
    {
        std::array<int32_t, 4> cpui;
        cpuid(&cpui, 0x80000000);
        return cpui[0] ^ 0x80000000;
    }

    void ProcessVendor(CpuInfo::CpuID::Info0& Info, core::StackString<16>& cpuName)
    {
        auto goat = NumExtended();
        goat = 0;
        // GO !
        cpuid(&Info, 0);

        std::array<char, 32> name;
        name.fill('\0');

        memcpy(name.data(), &Info.ebx.name_, sizeof(Info.ebx.name_));
        memcpy(name.data() + 4, &Info.edx.name_, sizeof(Info.edx.name_));
        memcpy(name.data() + 8, &Info.ecx.name_, sizeof(Info.ecx.name_));

        cpuName.set(name.data());
    }

    void ProcessCPUName(core::StackString<64>& cpuName)
    {
        CpuIDData data;
        
        std::array<char,64> name;
        name.fill('\0');
        
        cpuid(data, 0x80000002);
        memcpy(name.data(), data.data(), sizeof(data));

        cpuid(data, 0x80000003);
        memcpy(name.data() + sizeof(data), data.data(), sizeof(data));

        cpuid(data, 0x80000004);
        memcpy(name.data() + (2 * sizeof(data)), data.data(), sizeof(data));

        cpuName.set(name.data());
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
        int32_t num = safe_static_cast<int32_t>(Len / sizeof(_SYSTEM_LOGICAL_PROCESSOR_INFORMATION));

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
                            X_ASSERT(false, "Unexpected number of caches at level %d.", Cache.Level)(Count);
                        }
                    }
                    else {
                        X_ASSERT(false, "Unexpected cache level.")(Cache.Level);
                    }

                } break;
            }
        }
    }
}

X_NAMESPACE_END