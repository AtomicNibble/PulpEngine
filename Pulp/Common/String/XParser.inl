

X_NAMESPACE_BEGIN(core)


X_INLINE void XParser::setFlags(LexFlags flags)
{
	flags_ = flags;
}

X_INLINE XParser::LexFlags XParser::getFlags(void)
{
	return flags_;
}


X_NAMESPACE_END
