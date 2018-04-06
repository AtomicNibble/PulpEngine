

X_NAMESPACE_BEGIN(core)

X_INLINE const char* XParser::GetFileName(void) const
{
    if (scriptStack_.isNotEmpty()) {
        return scriptStack_.top()->GetFileName();
    }
    X_WARNING("Parser", "called 'GetFileName' on a parser without a valid file loaded");
    return "";
}

X_INLINE int32_t XParser::GetLineNumber(void) const
{
    if (scriptStack_.isNotEmpty()) {
        return scriptStack_.top()->GetLineNumber();
    }
    X_WARNING("Parser", "called 'GetLineNumber' on a parser without a valid file loaded");
    return 0;
}

X_INLINE void XParser::setFlags(LexFlags flags)
{
    flags_ = flags;
}

X_INLINE XParser::LexFlags XParser::getFlags(void)
{
    return flags_;
}

X_NAMESPACE_END
