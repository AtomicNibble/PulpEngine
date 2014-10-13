


X_INLINE const char* CpuInfo::GetCpuName(void) const
{
	return m_cpuName;
}

X_INLINE const char* CpuInfo::GetCpuVendor(void) const
{
	return m_cpuVendor;
}

X_INLINE unsigned int CpuInfo::GetCoreCount(void) const
{
	return m_coreCount;
}

X_INLINE unsigned int CpuInfo::GetLogicalProcessorCount(void) const
{
	return m_logicalProcessorCount;
}

X_INLINE const CpuInfo::CpuID::Info0& CpuInfo::GetInfoType0(void) const
{
	return m_info0;
}

X_INLINE const CpuInfo::CpuID::Info1& CpuInfo::GetInfoType1(void) const
{
	return m_info1;
}

X_INLINE const CpuInfo::CpuID::InfoEx0& CpuInfo::GetExtendedInfoType0(void) const
{
	return m_infoEx0;
}

X_INLINE const CpuInfo::CpuID::InfoEx1& CpuInfo::GetExtendedInfoType1(void) const
{
	return m_infoEx1;
}

X_INLINE unsigned int CpuInfo::GetL1CacheCount(void) const
{
	return m_cacheCount[0];
}

X_INLINE unsigned int CpuInfo::GetL2CacheCount(void) const
{
	return m_cacheCount[1];
}

X_INLINE unsigned int CpuInfo::GetL3CacheCount(void) const
{
	return m_cacheCount[2];
}

X_INLINE const CpuInfo::CacheInfo& CpuInfo::GetL1CacheInfo(unsigned int i) const
{
	return m_caches[0][i];
}

X_INLINE const CpuInfo::CacheInfo& CpuInfo::GetL2CacheInfo(unsigned int i) const
{
	return m_caches[1][i];
}

X_INLINE const CpuInfo::CacheInfo& CpuInfo::GetL3CacheInfo(unsigned int i) const
{
	return m_caches[2][i];
}
