

X_NAMESPACE_BEGIN(net)


X_INLINE CompPropType::Enum CompProp::getType(void) const
{
    return type_;
}

X_INLINE CompPropFlags CompProp::getFlags(void) const
{
    return flags_;
}

X_INLINE int32_t CompProp::getFieldOffset(void) const
{
    return fieldOffset_;
}

X_INLINE int32_t CompProp::getNumBits(void) const
{
    return numBits_;
}

// --------------------------------------------------------------------

X_INLINE size_t CompTable::numProps(void) const
{
    return props_.size();
}

X_INLINE const CompProp& CompTable::getProp(size_t idx) const
{
    return props_[idx];
}

X_INLINE const char* CompTable::getTableName(void) const
{
    return pTableName_;
}

X_NAMESPACE_END