

X_NAMESPACE_BEGIN(render)

namespace shader
{
    X_INLINE void XShaderParam::setName(const core::string& name)
    {
        name_ = name;
        nameHash_ = core::StrHash(name_.c_str(), name_.length());
    }

    X_INLINE void XShaderParam::setName(const char* pName)
    {
        name_ = pName;
        nameHash_ = core::StrHash(name_.c_str(), name_.length());
    }

    X_INLINE void XShaderParam::setUpdateRate(UpdateFreq::Enum updateRate)
    {
        updateRate_ = updateRate;
    }

    X_INLINE void XShaderParam::setType(ParamType::Enum type)
    {
        type_ = type;
    }

    X_INLINE void XShaderParam::setFlags(ParamFlags flags)
    {
        flags_ = flags;
    }

    X_INLINE void XShaderParam::setBindPoint(int32_t bindPoint)
    {
        bindPoint_ = safe_static_cast<int16_t>(bindPoint);
    }

    X_INLINE void XShaderParam::setSize(int32_t size)
    {
        numParameters_ = safe_static_cast<int16>((size + 15) >> 4);
    }

    X_INLINE const core::string& XShaderParam::getName(void) const
    {
        return name_;
    }

    X_INLINE const core::StrHash& XShaderParam::getNameHash(void) const
    {
        return nameHash_;
    }

    X_INLINE UpdateFreq::Enum XShaderParam::getUpdateRate(void) const
    {
        return updateRate_;
    }

    X_INLINE ParamType::Enum XShaderParam::getType(void) const
    {
        return type_;
    }

    X_INLINE ParamFlags XShaderParam::getFlags(void) const
    {
        return flags_;
    }

    X_INLINE int16_t XShaderParam::getBindPoint(void) const
    {
        return bindPoint_;
    }

    X_INLINE int16_t XShaderParam::getBindOffset(void) const
    {
        return bindPoint_ * 16;
    }

    X_INLINE int16_t XShaderParam::getNumVecs(void) const
    {
        return numParameters_;
    }

    // ================================================================================

    X_INLINE void XCBuffer::setName(const core::string& name)
    {
        name_ = name;
    }

    X_INLINE void XCBuffer::setName(const char* pName)
    {
        name_ = pName;
    }

    X_INLINE void XCBuffer::setUpdateRate(UpdateFreq::Enum updateRate)
    {
        updateRate_ = updateRate;
    }

    X_INLINE void XCBuffer::setSize(int16_t size)
    {
        size_ = size;
        cpuData_.resize(size);
    }

    X_INLINE void XCBuffer::setBindPointAndCount(int16_t bindPoint, int16_t bindCount)
    {
        bindPoint_ = bindPoint;
        bindCount_ = bindCount;
    }

    X_INLINE void XCBuffer::setParamGranularitys(size_t varGran)
    {
        params_.setGranularity(varGran);
    }

    X_INLINE void XCBuffer::setCpuDataVersion(int32_t version)
    {
        cpuDataVersion_ = version;
    }

    // <><><><>

    X_INLINE const core::string& XCBuffer::getName(void) const
    {
        return name_;
    }

    X_INLINE bool XCBuffer::requireManualUpdate(void) const
    {
        return paramFlags_.IsSet(ParamType::Unknown);
    }

    X_INLINE bool XCBuffer::containsOnlyFreq(UpdateFreq::Enum freq) const
    {
        auto flags = updateFeqFlags_;
        flags.Remove(freq);
        return updateFeqFlags_.IsSet(freq) && !flags.IsAnySet();
    }

    X_INLINE bool XCBuffer::containsUpdateFreqs(UpdateFreqFlags flags) const
    {
        return (updateFeqFlags_ & flags).IsAnySet();
    }

    X_INLINE bool XCBuffer::containsKnownParams(void) const
    {
        auto flags = paramFlags_;
        flags.Remove(ParamTypeFlags::Unknown);
        return flags.IsAnySet();
    }

    X_INLINE UpdateFreq::Enum XCBuffer::getUpdateFreg(void) const
    {
        return updateRate_;
    }

    X_INLINE int16_t XCBuffer::getBindSize(void) const
    {
        return size_;
    }

    X_INLINE int16_t XCBuffer::getBindPoint(void) const
    {
        return bindPoint_;
    }

    X_INLINE int16_t XCBuffer::getBindCount(void) const
    {
        return bindCount_;
    }

    X_INLINE int32_t XCBuffer::getParamCount(void) const
    {
        return safe_static_cast<int32_t>(params_.size());
    }

    X_INLINE typename XCBuffer::Hasher::HashVal XCBuffer::getHash(void) const
    {
        return hash_;
    }

    X_INLINE ParamTypeFlags XCBuffer::getParamFlags(void) const
    {
        return paramFlags_;
    }

    X_INLINE int32_t XCBuffer::getCpuDataVersion(void) const
    {
        return cpuDataVersion_;
    }

    X_INLINE const XShaderParam& XCBuffer::operator[](int32_t idx) const
    {
        return params_[idx];
    }

    X_INLINE XShaderParam& XCBuffer::operator[](int32_t idx)
    {
        return params_[idx];
    }

    X_INLINE const XCBuffer::DataArr& XCBuffer::getCpuData(void) const
    {
        return cpuData_;
    }

    X_INLINE XCBuffer::DataArr& XCBuffer::getCpuData(void)
    {
        return cpuData_;
    }

} // namespace shader

X_NAMESPACE_END
