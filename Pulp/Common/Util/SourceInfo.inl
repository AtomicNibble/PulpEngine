

inline SourceInfo::SourceInfo(const char* const module, const char* const file,
    int line, const char* const function,
    const char* const functionSignature) :
    pModule_(module),
    pFile_(file),
    pFunction_(function),
    pFunctionSignature_(functionSignature),
    line_(line)
{
}
