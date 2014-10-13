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
				uint32_t m_maxValidValue;					///< Maximum meaningful value for the info type parameter.
			} eax;

			struct EBX
			{
				uint32_t m_name;							///< Identification String (part 1).
			} ebx;

			struct ECX
			{
				uint32_t m_name;							///< Identification String (part 3).
			} ecx;

			struct EDX
			{
				uint32_t m_name;							///< Identification String (part 2).
			} edx;

			static_assert(sizeof(EAX) == 4, "EAX has wrong size.");
			static_assert(sizeof(EBX) == 4, "EBX has wrong size.");
			static_assert(sizeof(ECX) == 4, "ECX has wrong size.");
			static_assert(sizeof(EDX) == 4, "EDX has wrong size.");
		};

		/// Internal class representing general information (info type 1).
		struct Info1
		{
			struct EAX
			{
				uint32_t m_steppingID : 4;					///< Stepping ID.
				uint32_t m_model : 4;						///< CPU model.
				uint32_t m_family : 4;						///< CPU family.
				uint32_t m_processorType : 2;				///< CPU processor type.
				uint32_t m_reserved : 2;					///< Reserved.
				uint32_t m_extendedModel : 4;				///< CPU extended model.
				uint32_t m_extendedFamily : 8;				///< CPU extended family.
				uint32_t m_reserved2 : 4;					///< Reserved.
			} eax;

			struct EBX
			{
				uint32_t m_brandIndex : 8;					///< Brand index.
				uint32_t m_cflushCacheLine : 8;				///< The size of the CLFLUSH cache line, in quadwords.
				uint32_t m_logicalProcessorCount : 8;		///< Logical processor count.
				uint32_t m_apicId : 8;						///< Initial APIC ID.
			} ebx;

			struct ECX
			{
				uint32_t m_SSE3newInstructions : 1;			///< SSE3 new instructions.
				uint32_t m_reserved : 2;					///< Reserved.
				uint32_t m_monitorWait : 1;					///< MONITOR/MWAIT.
				uint32_t m_cplQualifiedStore : 1;			///< CPL Qualified Debug Store (Intel).
				uint32_t m_vmExtensions : 1;				///< Virtual machine extensions (Intel).
				uint32_t m_saferModeExtensions : 1;			///< Safer mode extensions (Intel).
				uint32_t m_speedStepTech : 1;				///< Enhanced Intel SpeedStep technology (Intel).
				uint32_t m_thermalMonitor : 1;				///< Thermal Monitor (Intel).
				uint32_t m_SSSE3 : 1;						///< Supplemental Streaming SIMD Extensions 3 (SSSE3).
				uint32_t m_l1ContextID : 1;					///< L1 context ID (Intel).
				uint32_t m_reserved2 : 1;					///< Reserved.
				uint32_t m_FMA : 1;							///< 256-bit FMA extensions (Intel).
				uint32_t m_CMPXCHG16B : 1;					///< CMPXCHG16B support.
				uint32_t m_xTPR : 1;						///< xTPR update control.
				uint32_t m_perfCapMSR : 1;					///< Perf/Debug capability MSR.
				uint32_t m_reserved3 : 2;					///< Reserved.
				uint32_t m_DCA : 1;							///< Direct cache access (DCA) support (Intel).
				uint32_t m_SSE41 : 1;						///< SSE4.1 extensions.
				uint32_t m_SSE42 : 1;						///< SSE4.2 extensions.
				uint32_t m_x2APIC : 1;						///< x2APIC support (Intel).
				uint32_t m_MOVBE : 1;						///< MOVBE support (Intel).
				uint32_t m_POPCNT : 1;						///< POPCNT instruction support.
				uint32_t m_reserved4 : 1;					///< Reserved.
				uint32_t m_AES : 1;							///< AES support (Intel).
				uint32_t m_XSAVE : 1;						///< XSAVE support (Intel).
				uint32_t m_OSXSAVE : 1;						///< OSXSAVE support (Intel).
				uint32_t m_AVX : 1;							///< 256-bit Intel advanced vector extensions (Intel).
				uint32_t m_reserved5 : 3;					///< Reserved.
			} ecx;

			struct EDX
			{
				uint32_t m_FPU : 1;							///< x87 FPU on chip.
				uint32_t m_VME : 1;							///< Virtual-8086 mode enhancement.
				uint32_t m_DE : 1;							///< Debugging extensions.
				uint32_t m_PSE : 1;							///< Page size extensions.
				uint32_t m_TSC : 1;							///< Time stamp counter.
				uint32_t m_MSR : 1;							///< RDMSR and WRMSR support.
				uint32_t m_PAE : 1;							///< Physical address extensions.
				uint32_t m_MCE : 1;							///< Machine check exception.
				uint32_t m_CX8 : 1;							///< CMPXCHG8B instruction.
				uint32_t m_APIC : 1;						///< APIC on chip.
				uint32_t m_reserved : 1;					///< Reserved.
				uint32_t m_SEP : 1;							///< SYSENTER and SYSEXIT.
				uint32_t m_MTRR : 1;						///< Memory type range registers.
				uint32_t m_PGE : 1;							///< PTE global bit.
				uint32_t m_MCA : 1;							///< Machine check architecture.
				uint32_t m_CMOV : 1;						///< Conditional move/compare instruction.
				uint32_t m_PAT : 1;							///< Page Attribute Table.
				uint32_t m_PSE36 : 1;						///< 36-bit page size extension.
				uint32_t m_PSN : 1;							///< Processor serial number.
				uint32_t m_CLFSH : 1;						///< CFLUSH instruction.
				uint32_t m_reserved2 : 1;					///< Reserved.
				uint32_t m_DS : 1;							///< Debug store.
				uint32_t m_ACPI : 1;						///< Thermal monitor and clock control.
				uint32_t m_MMX : 1;							///< MMX technology.
				uint32_t m_FXSR : 1;						///< FXSAVE/FXRSTOR.
				uint32_t m_SSE : 1;							///< SSE extensions.
				uint32_t m_SSE2 : 1;						///< SSE2 extensions.
				uint32_t m_SS : 1;							///< Self snoop.
				uint32_t m_HTT : 1;							///< Multi-threading.
				uint32_t m_TM : 1;							///< Thermal monitor.
				uint32_t m_reserved3 : 1;					///< Reserved.
				uint32_t m_PBE : 1;							///< Pending break enable.
			} edx;

			static_assert(sizeof(EAX) == 4, "EAX has wrong size.");
			static_assert(sizeof(EBX) == 4, "EBX has wrong size.");
			static_assert(sizeof(ECX) == 4, "ECX has wrong size.");
			static_assert(sizeof(EDX) == 4, "EDX has wrong size.");
		};

		/// Internal class representing extended information 0 (extended info type 0).
		struct InfoEx0
		{
			struct EAX
			{
				uint32_t m_maxValidValue;					///< Maximum meaningful value of info type for extended function CPUID information.
			} eax;

			struct EBX
			{
				uint32_t m_reserved0;						///< Reserved.
			} ebx;

			struct ECX
			{
				uint32_t m_reserved1;						///< Reserved.
			} ecx;

			struct EDX
			{
				uint32_t m_reserved2;						///< Reserved.
			} edx;

			static_assert(sizeof(EAX) == 4, "EAX has wrong size.");
			static_assert(sizeof(EBX) == 4, "EBX has wrong size.");
			static_assert(sizeof(ECX) == 4, "ECX has wrong size.");
			static_assert(sizeof(EDX) == 4, "EDX has wrong size.");
		};

		/// Internal class representing extended information 1 (extended info type 1).
		struct InfoEx1
		{
			struct EAX
			{
				uint32_t m_extendedFeatures;				///< Extended processor signature and extended feature bits.
			} eax;

			struct EBX
			{
				uint32_t m_reserved;						///< Reserved.
			} ebx;

			struct ECX
			{
				uint32_t m_LAHF : 1;						///< LAHF/SAHF available in 64-bit mode.
				uint32_t m_cmpLegacy : 1;					///< Core multi-processing legacy mode (CmpLegacy) (AMD).
				uint32_t m_SVM : 1;							///< Secure virtual machine (SVM) (AMD).
				uint32_t m_EAPICS : 1;						///< Extended APIC Register Space (ExtApicSpace) (AMD).
				uint32_t m_AltMovCr8 : 1;					///< AltMovCr8 (AMD).
				uint32_t m_LZCNT : 1;						///< LZCNT Support (AMD).
				uint32_t m_SSE4A : 1;						///< SSE4A instruction support (EXTRQ, INSERTQ, MOVNTSD, MOVNTSS) (AMD).
				uint32_t m_misalignedSSE : 1;				///< Misaligned SSE support mode available (AMD).
				uint32_t m_PREFETCH : 1;					///< PREFETCH and PREFETCHW support (AMD).
				uint32_t m_reserved2 : 3;					///< Reserved.
				uint32_t m_SKINITDEV : 1;					///< SKINIT and DEV support (AMD).
				uint32_t m_reserved3 : 19;					///< Reserved.
			} ecx;

			struct EDX
			{
				uint32_t m_reserved : 11;					///< Reserved.
				uint32_t m_SYSCALL : 1;						///< SYSCALL/SYSRET available in 64-bit mode.
				uint32_t m_reserved2 : 8;					///< Reserved.
				uint32_t m_EDB : 1;							///< Execute disable bit available.
				uint32_t m_reserved3 : 1;					///< Reserved.
				uint32_t m_EMMX : 1;						///< Extensions to MMX instructions (AMD).
				uint32_t m_reserved4 : 2;					///< Reserved.
				uint32_t m_FFXSR : 1;						///< FFXSR (AMD).
				uint32_t m_1GBPS : 1;						///< 1GB page support (AMD).
				uint32_t m_RDTSCP : 1;						///< RDTSCP support (AMD).
				uint32_t m_reserved5 : 1;					///< Reserved.
				uint32_t m_64bit : 1;						///< 64-bit technology available.
				uint32_t m_3DnowExt : 1;					///< 3DnowExt (AMD).
				uint32_t m_3Dnow : 1;						///< 3Dnow! instructions (AMD).
			} edx;

			static_assert(sizeof(EAX) == 4, "EAX has wrong size.");
			static_assert(sizeof(EBX) == 4, "EBX has wrong size.");
			static_assert(sizeof(ECX) == 4, "ECX has wrong size.");
			static_assert(sizeof(EDX) == 4, "EDX has wrong size.");
		};

		static_assert(sizeof(Info0) == 16, "Info0 has wrong size.");
		static_assert(sizeof(Info1) == 16, "Info1 has wrong size.");
		static_assert(sizeof(InfoEx0) == 16, "InfoEx0 has wrong size.");
		static_assert(sizeof(InfoEx1) == 16, "InfoEx1 has wrong size.");

		Info0 info0;
		Info1 info1;
		InfoEx0 infoEx0;
		InfoEx1 infoEx1;
	};

	static_assert(sizeof(CpuID) == 16, "CpuID has wrong size.");


	typedef PROCESSOR_CACHE_TYPE CacheType;

	/// Holds information about a particular cache.
	struct CacheInfo
	{
		unsigned int	m_size;
		unsigned short	m_lineSize;
		unsigned char	m_associativity;
		CacheType		m_type;
	};

public:
	/// Gathers information about the CPU using the CPUID instruction.
	CpuInfo(void);

	/// Returns the CPU name.
	X_INLINE const char* GetCpuName(void) const;

	/// Returns the CPU brand.
	X_INLINE const char* GetCpuVendor(void) const;

	/// Returns the number of available cores.
	X_INLINE unsigned int GetCoreCount(void) const;

	/// Returns the number of logical processors.
	X_INLINE unsigned int GetLogicalProcessorCount(void) const;

	/// Returns general CPU information.
	X_INLINE const CpuID::Info0& GetInfoType0(void) const;

	/// Returns general CPU information.
	X_INLINE const CpuID::Info1& GetInfoType1(void) const;

	/// Returns extended CPU information.
	X_INLINE const CpuID::InfoEx0& GetExtendedInfoType0(void) const;

	/// Returns extended CPU information.
	X_INLINE const CpuID::InfoEx1& GetExtendedInfoType1(void) const;

	/// Returns the number of L1 caches present.
	X_INLINE unsigned int GetL1CacheCount(void) const;

	/// Returns the number of L2 caches present.
	X_INLINE unsigned int GetL2CacheCount(void) const;

	/// Returns the number of L3 caches present.
	X_INLINE unsigned int GetL3CacheCount(void) const;

	/// Returns the i-th L1 cache info.
	X_INLINE const CacheInfo& GetL1CacheInfo(unsigned int i) const;

	/// Returns the i-th L2 cache info.
	X_INLINE const CacheInfo& GetL2CacheInfo(unsigned int i) const;

	/// Returns the i-th L3 cache info.
	X_INLINE const CacheInfo& GetL3CacheInfo(unsigned int i) const;

private:
	X_NO_ASSIGN(CpuInfo);
	X_NO_COPY(CpuInfo);

	char m_cpuVendor[16];
	char m_cpuName[64];

	CpuID::Info0 m_info0;
	CpuID::Info1 m_info1;
	CpuID::InfoEx0 m_infoEx0;
	CpuID::InfoEx1 m_infoEx1;

	uint32_t m_coreCount;
	uint32_t m_logicalProcessorCount;

	// number of caches in each hierarchy (L1, L2, L3)
	uint32_t m_cacheCount[3];

	// up to 64 cache infos for each cache in the hierarchy (L1, L2, L3)
	// e.g., a PC with 32 cores and hyper-threading returns 64 L1 cache infos.
	CacheInfo m_caches[3][MAX_CACHE_COUNT];
};


#include "Cpu.inl"


X_NAMESPACE_END


#endif // _X_CPUINFO_H_