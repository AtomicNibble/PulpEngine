X_NAMESPACE_BEGIN(core)


X_INLINE const char* CpuInfo::GetCpuName(void) const
{
    return cpuName_.c_str();
}

X_INLINE const char* CpuInfo::GetCpuVendor(void) const
{
    return cpuVendor_.c_str();
}

X_INLINE uint32_t CpuInfo::GetCoreCount(void) const
{
    return coreCount_;
}

X_INLINE uint32_t CpuInfo::GetLogicalProcessorCount(void) const
{
    return logicalProcessorCount_;
}

X_INLINE const CpuInfo::CpuID::Info0& CpuInfo::GetInfoType0(void) const
{
    return info0_;
}

X_INLINE const CpuInfo::CpuID::Info1& CpuInfo::GetInfoType1(void) const
{
    return info1_;
}

X_INLINE const CpuInfo::CpuID::InfoEx0& CpuInfo::GetExtendedInfoType0(void) const
{
    return infoEx0_;
}

X_INLINE const CpuInfo::CpuID::InfoEx1& CpuInfo::GetExtendedInfoType1(void) const
{
    return infoEx1_;
}

X_INLINE uint32_t CpuInfo::GetL1CacheCount(void) const
{
    return cacheCount_[0];
}

X_INLINE uint32_t CpuInfo::GetL2CacheCount(void) const
{
    return cacheCount_[1];
}

X_INLINE uint32_t CpuInfo::GetL3CacheCount(void) const
{
    return cacheCount_[2];
}

X_INLINE const CpuInfo::CacheInfo& CpuInfo::GetL1CacheInfo(unsigned int i) const
{
    return caches_[0][i];
}

X_INLINE const CpuInfo::CacheInfo& CpuInfo::GetL2CacheInfo(unsigned int i) const
{
    return caches_[1][i];
}

X_INLINE const CpuInfo::CacheInfo& CpuInfo::GetL3CacheInfo(unsigned int i) const
{
    return caches_[2][i];
}

X_NAMESPACE_END
