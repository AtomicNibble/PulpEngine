#pragma once

#ifndef _X_CPUINFO_H_
#define _X_CPUINFO_H_

X_NAMESPACE_BEGIN(core)

class CpuInfo
{
    /// The maximum number of caches per cache level (1, 2, 3) that can be handled.
    static const size_t MAX_CACHE_COUNT = 64;

public:
    /// From  http://msdn.microsoft.com/en-us/library/hskdteyh%28v=vs.100%29.aspx
    union CpuID
    {
        /// The type expected by the <tt>__cpuid</tt> intrinsics that aliases with the individual info structs.
        int as_array[4];

        /// Internal class representing general information (info type 0).
        struct Info0
        {
            struct EAX
            {
                uint32_t maxValidValue_; ///< Maximum meaningful value for the info type parameter.
            } eax;

            struct EBX
            {
                uint32_t name_; ///< Identification String (part 1).
            } ebx;

            struct ECX
            {
                uint32_t name_; ///< Identification String (part 3).
            } ecx;

            struct EDX
            {
                uint32_t name_; ///< Identification String (part 2).
            } edx;

            X_ENSURE_SIZE(EAX, 4);
            X_ENSURE_SIZE(EBX, 4);
            X_ENSURE_SIZE(ECX, 4);
            X_ENSURE_SIZE(EDX, 4);
        };

        /// Internal class representing general information (info type 1).
        struct Info1
        {
            struct EAX
            {
                uint32_t steppingID_ : 4;     ///< Stepping ID.
                uint32_t model_ : 4;          ///< CPU model.
                uint32_t family_ : 4;         ///< CPU family.
                uint32_t processorType_ : 2;  ///< CPU processor type.
                uint32_t reserved_ : 2;       ///< Reserved.
                uint32_t extendedModel_ : 4;  ///< CPU extended model.
                uint32_t extendedFamily_ : 8; ///< CPU extended family.
                uint32_t reserved2_ : 4;      ///< Reserved.
            } eax;

            struct EBX
            {
                uint32_t brandIndex_ : 8;            ///< Brand index.
                uint32_t cflushCacheLine_ : 8;       ///< The size of the CLFLUSH cache line, in quadwords.
                uint32_t logicalProcessorCount_ : 8; ///< Logical processor count.
                uint32_t apicId_ : 8;                ///< Initial APIC ID.
            } ebx;

            struct ECX
            {
                uint32_t SSE3newInstructions_ : 1; ///< SSE3 new instructions.
                uint32_t reserved_ : 2;            ///< Reserved.
                uint32_t monitorWait_ : 1;         ///< MONITOR/MWAIT.
                uint32_t cplQualifiedStore_ : 1;   ///< CPL Qualified Debug Store (Intel).
                uint32_t vmExtensions_ : 1;        ///< Virtual machine extensions (Intel).
                uint32_t saferModeExtensions_ : 1; ///< Safer mode extensions (Intel).
                uint32_t speedStepTech_ : 1;       ///< Enhanced Intel SpeedStep technology (Intel).
                uint32_t thermalMonitor_ : 1;      ///< Thermal Monitor (Intel).
                uint32_t SSSE3_ : 1;               ///< Supplemental Streaming SIMD Extensions 3 (SSSE3).
                uint32_t l1ContextID_ : 1;         ///< L1 context ID (Intel).
                uint32_t reserved2_ : 1;           ///< Reserved.
                uint32_t FMA_ : 1;                 ///< 256-bit FMA extensions (Intel).
                uint32_t CMPXCHG16B_ : 1;          ///< CMPXCHG16B support.
                uint32_t xTPR_ : 1;                ///< xTPR update control.
                uint32_t perfCapMSR_ : 1;          ///< Perf/Debug capability MSR.
                uint32_t reserved3_ : 2;           ///< Reserved.
                uint32_t DCA_ : 1;                 ///< Direct cache access (DCA) support (Intel).
                uint32_t SSE41_ : 1;               ///< SSE4.1 extensions.
                uint32_t SSE42_ : 1;               ///< SSE4.2 extensions.
                uint32_t x2APIC_ : 1;              ///< x2APIC support (Intel).
                uint32_t MOVBE_ : 1;               ///< MOVBE support (Intel).
                uint32_t POPCNT_ : 1;              ///< POPCNT instruction support.
                uint32_t reserved4_ : 1;           ///< Reserved.
                uint32_t AES_ : 1;                 ///< AES support (Intel).
                uint32_t XSAVE_ : 1;               ///< XSAVE support (Intel).
                uint32_t OSXSAVE_ : 1;             ///< OSXSAVE support (Intel).
                uint32_t AVX_ : 1;                 ///< 256-bit Intel advanced vector extensions (Intel).
                uint32_t reserved5_ : 3;           ///< Reserved.
            } ecx;

            struct EDX
            {
                uint32_t FPU_ : 1;       ///< x87 FPU on chip.
                uint32_t VME_ : 1;       ///< Virtual-8086 mode enhancement.
                uint32_t DE_ : 1;        ///< Debugging extensions.
                uint32_t PSE_ : 1;       ///< Page size extensions.
                uint32_t TSC_ : 1;       ///< Time stamp counter.
                uint32_t MSR_ : 1;       ///< RDMSR and WRMSR support.
                uint32_t PAE_ : 1;       ///< Physical address extensions.
                uint32_t MCE_ : 1;       ///< Machine check exception.
                uint32_t CX8_ : 1;       ///< CMPXCHG8B instruction.
                uint32_t APIC_ : 1;      ///< APIC on chip.
                uint32_t reserved_ : 1;  ///< Reserved.
                uint32_t SEP_ : 1;       ///< SYSENTER and SYSEXIT.
                uint32_t MTRR_ : 1;      ///< Memory type range registers.
                uint32_t PGE_ : 1;       ///< PTE global bit.
                uint32_t MCA_ : 1;       ///< Machine check architecture.
                uint32_t CMOV_ : 1;      ///< Conditional move/compare instruction.
                uint32_t PAT_ : 1;       ///< Page Attribute Table.
                uint32_t PSE36_ : 1;     ///< 36-bit page size extension.
                uint32_t PSN_ : 1;       ///< Processor serial number.
                uint32_t CLFSH_ : 1;     ///< CFLUSH instruction.
                uint32_t reserved2_ : 1; ///< Reserved.
                uint32_t DS_ : 1;        ///< Debug store.
                uint32_t ACPI_ : 1;      ///< Thermal monitor and clock control.
                uint32_t MMX_ : 1;       ///< MMX technology.
                uint32_t FXSR_ : 1;      ///< FXSAVE/FXRSTOR.
                uint32_t SSE_ : 1;       ///< SSE extensions.
                uint32_t SSE2_ : 1;      ///< SSE2 extensions.
                uint32_t SS_ : 1;        ///< Self snoop.
                uint32_t HTT_ : 1;       ///< Multi-threading.
                uint32_t TM_ : 1;        ///< Thermal monitor.
                uint32_t reserved3_ : 1; ///< Reserved.
                uint32_t PBE_ : 1;       ///< Pending break enable.
            } edx;

            X_ENSURE_SIZE(EAX, 4);
            X_ENSURE_SIZE(EBX, 4);
            X_ENSURE_SIZE(ECX, 4);
            X_ENSURE_SIZE(EDX, 4);
        };

        /// Internal class representing extended information 0 (extended info type 0).
        struct InfoEx0
        {
            struct EAX
            {
                uint32_t maxValidValue_; ///< Maximum meaningful value of info type for extended function CPUID information.
            } eax;

            struct EBX
            {
                uint32_t reserved0_; ///< Reserved.
            } ebx;

            struct ECX
            {
                uint32_t reserved1_; ///< Reserved.
            } ecx;

            struct EDX
            {
                uint32_t reserved2_; ///< Reserved.
            } edx;

            X_ENSURE_SIZE(EAX, 4);
            X_ENSURE_SIZE(EBX, 4);
            X_ENSURE_SIZE(ECX, 4);
            X_ENSURE_SIZE(EDX, 4);
        };

        /// Internal class representing extended information 1 (extended info type 1).
        struct InfoEx1
        {
            struct EAX
            {
                uint32_t extendedFeatures_; ///< Extended processor signature and extended feature bits.
            } eax;

            struct EBX
            {
                uint32_t reserved_; ///< Reserved.
            } ebx;

            struct ECX
            {
                uint32_t LAHF_ : 1;          ///< LAHF/SAHF available in 64-bit mode.
                uint32_t cmpLegacy_ : 1;     ///< Core multi-processing legacy mode (CmpLegacy) (AMD).
                uint32_t SVM_ : 1;           ///< Secure virtual machine (SVM) (AMD).
                uint32_t EAPICS_ : 1;        ///< Extended APIC Register Space (ExtApicSpace) (AMD).
                uint32_t AltMovCr8_ : 1;     ///< AltMovCr8 (AMD).
                uint32_t LZCNT_ : 1;         ///< LZCNT Support (AMD).
                uint32_t SSE4A_ : 1;         ///< SSE4A instruction support (EXTRQ, INSERTQ, MOVNTSD, MOVNTSS) (AMD).
                uint32_t misalignedSSE_ : 1; ///< Misaligned SSE support mode available (AMD).
                uint32_t PREFETCH_ : 1;      ///< PREFETCH and PREFETCHW support (AMD).
                uint32_t reserved2_ : 3;     ///< Reserved.
                uint32_t SKINITDEV_ : 1;     ///< SKINIT and DEV support (AMD).
                uint32_t reserved3_ : 19;    ///< Reserved.
            } ecx;

            struct EDX
            {
                uint32_t reserved_ : 11;   ///< Reserved.
                uint32_t SYSCALL_ : 1;     ///< SYSCALL/SYSRET available in 64-bit mode.
                uint32_t reserved2_ : 8;   ///< Reserved.
                uint32_t EDB_ : 1;         ///< Execute disable bit available.
                uint32_t reserved3_ : 1;   ///< Reserved.
                uint32_t EMMX_ : 1;        ///< Extensions to MMX instructions (AMD).
                uint32_t reserved4_ : 2;   ///< Reserved.
                uint32_t FFXSR_ : 1;       ///< FFXSR (AMD).
                uint32_t amd1GBPS_ : 1;    ///< 1GB page support (AMD).
                uint32_t RDTSCP_ : 1;      ///< RDTSCP support (AMD).
                uint32_t reserved5_ : 1;   ///< Reserved.
                uint32_t is64bit_ : 1;     ///< 64-bit technology available.
                uint32_t amd3DnowExt_ : 1; ///< 3DnowExt (AMD).
                uint32_t amd3Dnow_ : 1;    ///< 3Dnow! instructions (AMD).
            } edx;

            X_ENSURE_SIZE(EAX, 4);
            X_ENSURE_SIZE(EBX, 4);
            X_ENSURE_SIZE(ECX, 4);
            X_ENSURE_SIZE(EDX, 4);
        };

        X_ENSURE_SIZE(Info0, 16);
        X_ENSURE_SIZE(Info1, 16);
        X_ENSURE_SIZE(InfoEx0, 16);
        X_ENSURE_SIZE(InfoEx1, 16);

        Info0 info0;
        Info1 info1;
        InfoEx0 infoEx0;
        InfoEx1 infoEx1;
    };

    X_ENSURE_SIZE(CpuID, 16);

    typedef PROCESSOR_CACHE_TYPE CacheType;

    /// Holds information about a particular cache.
    struct CacheInfo
    {
        uint32_t size_;
        uint16_t lineSize_;
        uint8_t associativity_;
        CacheType type_;
    };

public:
    /// Gathers information about the CPU using the CPUID instruction.
    CpuInfo(void);

    /// Returns the CPU name.
    X_INLINE const char* GetCpuName(void) const;

    /// Returns the CPU brand.
    X_INLINE const char* GetCpuVendor(void) const;

    /// Returns the number of available cores.
    X_INLINE uint32_t GetCoreCount(void) const;

    /// Returns the number of logical processors.
    X_INLINE uint32_t GetLogicalProcessorCount(void) const;

    /// Returns general CPU information.
    X_INLINE const CpuID::Info0& GetInfoType0(void) const;

    /// Returns general CPU information.
    X_INLINE const CpuID::Info1& GetInfoType1(void) const;

    /// Returns extended CPU information.
    X_INLINE const CpuID::InfoEx0& GetExtendedInfoType0(void) const;

    /// Returns extended CPU information.
    X_INLINE const CpuID::InfoEx1& GetExtendedInfoType1(void) const;

    /// Returns the number of L1 caches present.
    X_INLINE uint32_t GetL1CacheCount(void) const;

    /// Returns the number of L2 caches present.
    X_INLINE uint32_t GetL2CacheCount(void) const;

    /// Returns the number of L3 caches present.
    X_INLINE uint32_t GetL3CacheCount(void) const;

    /// Returns the i-th L1 cache info.
    X_INLINE const CacheInfo& GetL1CacheInfo(unsigned int i) const;

    /// Returns the i-th L2 cache info.
    X_INLINE const CacheInfo& GetL2CacheInfo(unsigned int i) const;

    /// Returns the i-th L3 cache info.
    X_INLINE const CacheInfo& GetL3CacheInfo(unsigned int i) const;

private:
    X_NO_ASSIGN(CpuInfo);
    X_NO_COPY(CpuInfo);

    core::StackString<16> cpuVendor_;
    core::StackString<64> cpuName_;

    CpuID::Info0 info0_;
    CpuID::Info1 info1_;
    CpuID::InfoEx0 infoEx0_;
    CpuID::InfoEx1 infoEx1_;

    uint32_t coreCount_;
    uint32_t logicalProcessorCount_;

    // number of caches in each hierarchy (L1, L2, L3)
    uint32_t cacheCount_[3];

    // up to 64 cache infos for each cache in the hierarchy (L1, L2, L3)
    // e.g., a PC with 32 cores and hyper-threading returns 64 L1 cache infos.
    CacheInfo caches_[3][MAX_CACHE_COUNT];
};


X_NAMESPACE_END

#include "Cpu.inl"

#endif // _X_CPUINFO_H_