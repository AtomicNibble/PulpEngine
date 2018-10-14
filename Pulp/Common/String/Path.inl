
template<typename TChar>
Path<TChar>::Path()
{
}

template<typename TChar>
Path<TChar>::Path(const Path& oth) :
    BaseType(oth.begin(), oth.end())
{
}

template<>
template<>
inline Path<char>::Path(const Path<wchar_t>& oth) :
    BaseType(oth.begin(), oth.end())
{
}

template<>
template<>
inline Path<wchar_t>::Path(const Path<char>& oth) :
     BaseType(oth.begin(), oth.end())
{
}

template<>
template<>
inline Path<char>::Path(const wchar_t* const beginInclusive, const wchar_t* const endExclusive) :
    BaseType(beginInclusive, endExclusive)
{
}

template<>
template<>
inline Path<wchar_t>::Path(const char* const beginInclusive, const char* const endExclusive) :
    BaseType(beginInclusive, endExclusive)
{
}


template<typename TChar>
Path<TChar>::Path(const TChar* const str) :
    BaseType(str)
{
}

template<typename TChar>
Path<TChar>::Path(const TChar* const beginInclusive, const TChar* const endExclusive) :
    BaseType(beginInclusive, endExclusive)
{
}

template<>
template<>
inline void Path<char>::set(const wchar_t* const beginInclusive, const wchar_t* const endExclusive)
{
    len_ = safe_static_cast<size_t>(endExclusive - beginInclusive);
    strUtil::Convert(beginInclusive, str_, BaseType::capacity());
    str_[len_] = L'\0';
}

template<>
template<>
inline void Path<wchar_t>::set(const char* const beginInclusive, const char* const endExclusive)
{
    len_ = safe_static_cast<size_t>(endExclusive - beginInclusive);
    strUtil::Convert(beginInclusive, str_, BaseType::capacity());
    str_[len_] = L'\0';
}

template<>
template<>
inline void Path<char>::append(const wchar_t* const beginInclusive, const wchar_t* const endExclusive)
{
    auto len = safe_static_cast<size_t>(endExclusive - beginInclusive);
    strUtil::Convert(beginInclusive, &str_[len_], BaseType::freeSpace());
    len_ += len;
    str_[len_] = L'\0';
}

template<>
template<>
inline void Path<wchar_t>::append(const char* const beginInclusive, const char* const endExclusive)
{
    auto len = safe_static_cast<size_t>(endExclusive - beginInclusive);
    strUtil::Convert(beginInclusive, &str_[len_], BaseType::freeSpace());
    len_ += len;
    str_[len_] = L'\0';
}


template<typename TChar>
const TChar* Path<TChar>::fileName(void) const
{
    const TChar* native = BaseType::findLast(NATIVE_SLASH);
    // folder//
    if (native == BaseType::end() - 1) {
        return native + 1;
    }

    const TChar* noneNative = BaseType::findLast(NON_NATIVE_SLASH);
    // folder
    if (noneNative == BaseType::end() - 1) {
        return noneNative + 1;
    }

    if (!native && !noneNative) {
        return BaseType::begin();
    }
    if (!noneNative && native) {
        return native + 1;
    }
    if (noneNative && !native) {
        return noneNative + 1;
    }

    // work out which is bigger
    const TChar* last = core::Max(native, noneNative);

    return last + 1;
}

template<typename TChar>
const TChar* Path<TChar>::extension(bool incDot) const
{
    const TChar* res = BaseType::findLast('.');

    if (!res) {
        return BaseType::begin();
    }

    if (incDot) {
        return res;
    }
    return res + 1;
}
template<typename TChar>
void Path<TChar>::setExtension(const TChar* pExtension)
{
    const TChar* remove = BaseType::findLast('.'); // need to remvoe a extension?
    bool has_dot = (pExtension[0] == '.');         // new extension got a dot?
    bool is_blank = (pExtension[0] == '\0');       //

    if (remove) {
        BaseType::trimRight(remove);
    }

    if (!is_blank) {
        if (!has_dot) {
            BaseType::append('.', 1);
        }

        BaseType::append(pExtension);
    }
}

template<typename TChar>
void Path<TChar>::setFileName(const TChar* pFilename)
{
    // place in temp buffer otherwise
    // the original file name would be update
    // while we are replacing.

    const TChar* name = fileName();

    if (BaseType::isEmpty() || (name == BaseType::end())) {
        BaseType::append(pFilename);
    }
    else {
        BaseType temp(BaseType::str_, name); // want the text before filename
        temp.append(pFilename);

        *this = temp.c_str();
    }
}

template<typename TChar>
void Path<TChar>::setFileName(const TChar* pFileNameBegin, const TChar* pFileNameEnd)
{
    // place in temp buffer otherwise
    // the original file name would be update
    // while we are replacing.

    const TChar* name = fileName();

    if (BaseType::isEmpty() || (name == BaseType::end())) {
        BaseType::append(pFileNameBegin, pFileNameEnd);
    }
    else {
        BaseType temp(BaseType::str_, name); // want the text before filename
        temp.append(pFileNameBegin, pFileNameEnd);

        *this = temp.c_str();
    }
}

template<typename TChar>
void Path<TChar>::operator=(const TChar* str)
{
    BaseType::len_ = strUtil::strlen(str);
    BaseType::len_ = core::Min<size_t>(BaseType::len_, MAX_PATH);
    memcpy(BaseType::str_, str, (BaseType::len_ + 1) * sizeof(TChar));
}

// -----------------------------------------------

template<typename TChar>
const Path<TChar> Path<TChar>::operator/(const Path<TChar>& oth) const
{
    Path path(*this);
    path.ensureSlash();
    path.append(oth.c_str());
    return path;
}

template<typename TChar>
const Path<TChar> Path<TChar>::operator/(const TChar* str) const
{
    Path<TChar> path(*this);
    path.ensureSlash();
    path.append(str);
    return path;
}

template<typename TChar>
const Path<TChar>& Path<TChar>::operator/=(const Path<TChar>& oth)
{
    ensureSlash();
    BaseType::append(oth.c_str(), oth.length());
    return *this;
}

template<typename TChar>
const Path<TChar>& Path<TChar>::operator/=(const TChar* str)
{
    ensureSlash();
    BaseType::append(str);
    return *this;
}

// -----------------------------------------------

template<typename TChar>
const Path<TChar> Path<TChar>::operator+(const Path<TChar>& oth) const
{
    Path path(*this);
    path.append(oth.c_str());
    return path;
}

template<typename TChar>
const Path<TChar> Path<TChar>::operator+(const TChar* str) const
{
    Path<TChar> path(*this);
    path.append(str);
    return path;
}

template<typename TChar>
const Path<TChar>& Path<TChar>::operator+=(const Path<TChar>& oth)
{
    BaseType::append(oth.c_str(), oth.length());
    return *this;
}

template<typename TChar>
const Path<TChar>& Path<TChar>::operator+=(const TChar* str)
{
    BaseType::append(str);
    return *this;
}

// -----------------------------------------------

template<typename TChar>
inline void Path<TChar>::ensureSlash(void)
{
    if (BaseType::len_ > 0) {
        BaseType::stripTrailing(NATIVE_SLASH);
        BaseType::append(NATIVE_SLASH, 1);
    }
}

template<>
inline void Path<char>::replaceSeprators(void)
{
    replaceAll(NON_NATIVE_SLASH, NATIVE_SLASH);
}

template<>
inline void Path<wchar_t>::replaceSeprators(void)
{
    replaceAll(NON_NATIVE_SLASH_W, NATIVE_SLASH_W);
}

template<>
inline void Path<char>::removeFileName(void)
{
    this->replace(this->fileName(), "");
}

template<>
inline void Path<wchar_t>::removeFileName(void)
{
    this->replace(this->fileName(), L"");
}

template<>
inline void Path<char>::removeExtension(void)
{
    if (isNotEmpty()) {
        setExtension("");
    }
}

template<>
inline void Path<wchar_t>::removeExtension(void)
{
    if (isNotEmpty()) {
        setExtension(L"");
    }
}

template<typename TChar>
inline void Path<TChar>::removeTrailingSlash(void)
{
    replaceSeprators();
    BaseType::stripTrailing(NATIVE_SLASH);
}

template<typename TChar>
inline size_t Path<TChar>::fillSpaceWithNullTerm(void)
{
    const size_t space = BaseType::capacity() - BaseType::length();

    std::memset(&BaseType::str_[BaseType::len_], '\0', space);

    return space;
}

template<typename TChar>
inline bool Path<TChar>::isAbsolute(void) const
{
    const auto len = length();

    // https://docs.microsoft.com/en-us/dotnet/standard/io/file-path-formats
    // \folder is absolute
    if (len > 0 && (BaseType::str_[0] == NATIVE_SLASH || BaseType::str_[0] == NON_NATIVE_SLASH)) {
        return true;
    }

    // c:\folder is absolute c:folder is not
    if (len > 2 && BaseType::str_[1] == ':' && (BaseType::str_[2] == NATIVE_SLASH || BaseType::str_[2] == NON_NATIVE_SLASH)) {
        return true;
    }

    return false;
}

template<typename TChar>
inline int8_t Path<TChar>::getDriveNumber(void) const
{
    if (BaseType::length() > 1 && BaseType::str_[1] == ':') {
        return safe_static_cast<int8_t, int32_t>(BaseType::str_[0] - 'A');
    }

    return -1;
}