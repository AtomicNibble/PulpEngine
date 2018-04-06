

X_INLINE const char* SymbolInfo::GetFunction(void) const
{
    return function_.c_str();
}

X_INLINE const char* SymbolInfo::GetFilename(void) const
{
    return filename_.c_str();
}

X_INLINE unsigned int SymbolInfo::GetLine(void) const
{
    return line_;
}
