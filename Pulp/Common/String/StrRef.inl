

template<typename CharT>
StringRef<CharT>::StringRef()
{
    SetEmpty();
}

// from another string object
template<typename CharT>
StringRef<CharT>::StringRef(const StrT& str)
{
    X_ASSERT(str.header()->refCount != 0, "can't constuct a string from one that has a ref count of 0")
    ();

    if (str.header()->refCount >= 0) {
        str_ = str.str_;
        header()->addRef();
    }
    else {
        SetEmpty();
    }
}

// from another string object
template<typename CharT>
StringRef<CharT>::StringRef(StrT&& oth)
{
    str_ = oth.str_;

    oth.SetEmpty();
}

// from another string object, define the offset and count
template<typename CharT>
StringRef<CharT>::StringRef(const StrT& str, size_type offset, size_type count)
{
    SetEmpty();
    assign(str, offset, count);
}

// const from char (optionaly repeat it x number of times)
template<typename CharT>
StringRef<CharT>::StringRef(value_type ch, size_type numRepeat)
{
    SetEmpty(); // if for some reason repeat is 0 (maybe assert on it?)
    X_ASSERT(numRepeat > 0, "string constructed with a char repeat value of 0.")
    (ch, numRepeat);

    if (numRepeat > 0) {
        Allocate(numRepeat);
        _set(str_, ch, numRepeat);
    }
}

// from a const str (allocates memory & copyies)
template<typename CharT>
StringRef<CharT>::StringRef(const_str str)
{
    SetEmpty();
    size_t len = strlen(str);
    if (len > 0) {
        Allocate(len);
        _copy(str_, str, len);
    }
}

// const from string + length saves a length
template<typename CharT>
StringRef<CharT>::StringRef(const_str str, size_type length)
{
    SetEmpty();
    size_t len = length;
    if (len > 0) {
        Allocate(len);
        _copy(str_, str, len);
    }
}

// const from a begin(), end() saves a length cal
template<typename CharT>
StringRef<CharT>::StringRef(const_iterator first, const_iterator last)
{
    SetEmpty();
    size_t len = (last - first);
    if (len > 0) {
        Allocate(len);
        _copy(str_, first, len);
    }
}

template<typename CharT>
StringRef<CharT>::StringRef(const ConstCharWrapper& str)
{
    SetEmpty();
    X_ASSERT_NOT_NULL(str.getCharPtr());
    str_ = const_cast<pointer>(str.getCharPtr());
}

// removes a refrence from the string, delete if == 0
template<typename CharT>
StringRef<CharT>::~StringRef()
{
    // de-inc ref count.
    // no need to assing empty string (aka free())
    // as this object is dead.
    freeData(header());

    // in debug builds check for double deconstruction.
#if X_DEBUG
    str_ = nullptr;
#endif // !X_DEBUG
}

// =============================================================================

template<typename CharT>
StringRef<CharT>::operator const_str(void) const
{
    return str_;
}

template<typename CharT>
typename StringRef<CharT>::const_pointer StringRef<CharT>::c_str(void) const
{
    return str_;
}

template<typename CharT>
typename StringRef<CharT>::const_pointer StringRef<CharT>::data(void) const
{
    return str_;
}

// iterator
template<typename CharT>
typename StringRef<CharT>::const_iterator StringRef<CharT>::begin(void) const
{
    return str_;
}

template<typename CharT>
typename StringRef<CharT>::const_iterator StringRef<CharT>::end(void) const
{
    return str_ + length();
}

// =============================================================================

// the length of the string
template<typename CharT>
typename StringRef<CharT>::size_type StringRef<CharT>::length(void) const
{
    return safe_static_cast<StringRef<CharT>::size_type, StringRef<CharT>::length_type>(header()->length);
}

// same as length
template<typename CharT>
typename StringRef<CharT>::size_type StringRef<CharT>::size(void) const
{
    return length();
}

// returns current size of allocated memory
template<typename CharT>
typename StringRef<CharT>::size_type StringRef<CharT>::capacity(void) const
{
    return safe_static_cast<StringRef<CharT>::size_type, StringRef<CharT>::length_type>(header()->allocSize);
}

template<typename CharT>
bool StringRef<CharT>::isEmpty(void) const
{
    return length() == 0;
}

template<typename CharT>
bool StringRef<CharT>::isNotEmpty(void) const
{
    return length() > 0;
}

// clears the string and de-inc the ref
template<typename CharT>
void StringRef<CharT>::clear(void)
{
    if (length() == 0)
        return;
    if (header()->refCount >= 0) // check for the -1 ref.
        free();

    X_ASSERT(length() == 0, "Failed to clear string")
    (length());
    X_ASSERT(header()->refCount < 0 || capacity() == 0, "string is not pointing to empty data")
    ();
}

// Sets the capacity of the string to a number at least as great as a specified number.
template<typename CharT>
void StringRef<CharT>::reserve(size_type size)
{
    if (size > capacity()) {
        XStrHeader* pOldData = header();
        Allocate(size);
        _copy(str_, pOldData->getChars(), pOldData->length);
        header()->length = pOldData->length;
        str_[pOldData->length] = 0;
        freeData(pOldData);
    }
}

template<class T>
inline void StringRef<T>::resize(size_type size, value_type ch)
{
    makeUnique();
    if (size > length()) // bigger or less?
    {
        size_type numToAdd = size - length();
        append(numToAdd, ch);
    }
    else if (size < length()) // smaller than current?
    {
        header()->length = safe_static_cast<int, size_t>(size);
        str_[length()] = 0; // Make null terminated string.
    }
}

// shrink memory allocated to equal the length of the string.
template<typename CharT>
void StringRef<CharT>::shrinkToFit(void)
{
    if (length() != capacity()) {
        XStrHeader* pOldData = header();
        Allocate(length());
        _copy(str_, pOldData->getChars(), pOldData->length);
        freeData(pOldData);
    }
}

// =============================================================================

// overloaded assignment
template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::operator=(const StrT& str)
{
    if (str_ != str.str_) // same string? O<.>O!
    {
        int thisRef = header()->refCount;
        int srcRef = str.header()->refCount;

        // is this string null
        if (thisRef < 0) {
            if (srcRef < 0) {
                // both are null :|
                // what do you want from me!
            }
            else {
                str_ = str.str_;
                header()->addRef();
            }
        }
        else if (srcRef < 0) {
            free();
            str_ = str.str_;
        }
        else {
            // free the current string, and set + ref.
            free();
            str_ = str.str_;
            header()->addRef();
        }
    }
    return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::operator=(StrT&& oth)
{
    if (this != &oth) {
        freeData(header());

        str_ = oth.str_;
        oth.SetEmpty();
    }
    return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::operator=(value_type ch)
{
    // single char for my booty.
    _Assign(&ch, 1);
    return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::operator=(const_str str)
{
    // we don't allow null pointer to be assigned
    X_ASSERT_NOT_NULL(str);

    _Assign(str, strlen(str));
    return *this;
}

// string concatenation
template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::operator+=(const StrT& str)
{
    ConcatenateInPlace(str.c_str(), str.length());
    return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::operator+=(value_type ch)
{
    ConcatenateInPlace(&ch, 1);
    return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::operator+=(const_str str)
{
    X_ASSERT_NOT_NULL(str);

    ConcatenateInPlace(str, strlen(str));
    return *this;
}

// =============================================================================

// Case util
template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::toLower(void)
{
    makeUnique();
    size_type i, len;

    len = length();
    for (i = 0; i < len; ++i) {
        str_[i] = safe_static_cast<CharT>(tolower(str_[i]));
    }
    return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::toUpper(void)
{
    makeUnique();
    size_type i, len;

    len = length();
    for (i = 0; i < len; ++i) {
        str_[i] = safe_static_cast<CharT>(toupper(str_[i]));
    }
    return *this;
}

// =============================================================================

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trim(void)
{
    return trimRight().trimLeft();
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trim(value_type ch)
{
    makeUnique();

    const value_type temp[2] = {ch, 0};
    return trimRight(temp).trimLeft(temp);
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trim(const_str charSet)
{
    makeUnique();

    return trimRight(charSet).trimLeft(charSet);
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trimLeft(void)
{
    const value_type* str = str_;
    while ((*str != 0) && strUtil::IsWhitespace((unsigned char)*str))
        str++;

    if (str != str_) {
        size_type nOff = (size_type)(str - str_); // m_str can change in _MakeUnique
        makeUnique();
        size_type nNewLength = length() - nOff;
        _move(str_, str_ + nOff, nNewLength + 1);
        header()->length = safe_static_cast<length_type, size_type>(nNewLength);
        str_[nNewLength] = 0;
    }

    return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trimLeft(value_type ch)
{
    const value_type temp[2] = {ch, 0};
    return trimLeft(temp);
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trimLeft(const_str charSet)
{
    if (!charSet || !(*charSet))
        return *this;

    const_str charSetend = charSet + strlen(charSet);

    const_str str = str_;
    while ((*str != 0) && (strUtil::Find(charSet, charSetend, *str) != nullptr))
        str++;

    if (str != str_) {
        size_type nOff = (size_type)(str - str_); // str_ can change in _MakeUnique
        makeUnique();
        size_type nNewLength = length() - nOff;
        _move(str_, str_ + nOff, nNewLength + 1);
        header()->length = safe_static_cast<length_type, size_type>(nNewLength);
        str_[nNewLength] = 0;
    }

    return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trimRight(void)
{
    if (length() < 1)
        return *this;

    const value_type* last = str_ + length() - 1;
    const value_type* str = last;
    while ((str != str_) && strUtil::IsWhitespace((unsigned char)*str))
        str--;

    if (str != last) // something changed?
    {
        // Just shrink length of the string.
        size_type nNewLength = (size_type)(str - str_) + 1; // str_ can change in _MakeUnique
        makeUnique();
        header()->length = safe_static_cast<length_type, size_type>(nNewLength);
        str_[nNewLength] = 0;
    }

    return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trimRight(value_type ch)
{
    const value_type temp[2] = {ch, 0};
    return trimRight(temp);
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trimRight(const_str charSet)
{
    if (!charSet || !(*charSet) || length() < 1)
        return *this;

    const_str charSetend = charSet + strlen(charSet);
    const_str last = str_ + length() - 1;
    const_str str = last;
    while ((str != str_) && (strUtil::Find(charSet, charSetend, *str) != nullptr))
        str--;

    if (str != last) {
        // Just shrink length of the string.
        size_type nNewLength = (size_type)(str - str_) + 1; // str_ can change in _MakeUnique
        makeUnique();
        header()->length = safe_static_cast<length_type, size_type>(nNewLength);
        str_[nNewLength] = 0;
    }

    return *this;
}

// =============================================================================

template<class CharT>
inline StringRef<CharT>& StringRef<CharT>::append(const_str _Ptr)
{
    *this += _Ptr;
    return *this;
}

template<class CharT>
inline StringRef<CharT>& StringRef<CharT>::append(const_str _Ptr, size_type count)
{
    ConcatenateInPlace(_Ptr, count);
    return *this;
}

template<class CharT>
inline StringRef<CharT>& StringRef<CharT>::append(const StringRef<CharT>& _Str,
    size_type off, size_type count)
{
    size_type len = _Str.length();
    if (off > len)
        return *this;
    if (off + count > len)
        count = len - off;
    ConcatenateInPlace(_Str.str_ + off, count);
    return *this;
}

template<class CharT>
inline StringRef<CharT>& StringRef<CharT>::append(const StringRef<CharT>& _Str)
{
    *this += _Str;
    return *this;
}

template<class D, class S>
inline D& check_convert(D& d, S const& s)
{
    d = D(s);
    return d;
}

// Convert one type to another, asserting there is no conversion loss.
// Usage: DestType dest;  check_convert(dest) = src;
template<class D>
struct CheckConvert
{
    CheckConvert(D& d) :
        dest(&d)
    {
    }

    template<class S>
    D& operator=(S const& s)
    {
        return check_convert(*dest, s);
    }

protected:
    D* dest;
};

template<class D>
inline CheckConvert<D> check_convert(D& d)
{
    return d;
}

template<class CharT>
inline StringRef<CharT>& StringRef<CharT>::append(size_type count, value_type _Ch)
{
    if (count > 0) {
        if (length() + count >= capacity()) {
            XStrHeader* pOldData = header();
            Allocate(length() + count);
            _copy(str_, pOldData->getChars(), pOldData->length);
            _set(str_ + pOldData->length, _Ch, count);
            freeData(pOldData);
        }
        else {
            size_type nOldLength = length();
            _set(str_ + nOldLength, _Ch, count);
            check_convert(header()->length) = nOldLength + count;
            str_[length()] = 0; // Make null terminated string.
        }
    }
    return *this;
}

template<class CharT>
inline StringRef<CharT>& StringRef<CharT>::append(const_iterator _First, const_iterator _Last)
{
    append(_First, (size_type)(_Last - _First));
    return *this;
}

// =============================================================================

template<class CharT>
inline StringRef<CharT>& StringRef<CharT>::assign(const_str _Ptr)
{
    *this = _Ptr;
    return *this;
}

template<class CharT>
inline StringRef<CharT>& StringRef<CharT>::assign(const_str _Ptr, size_type count)
{
    size_type len = strnlen(_Ptr, count);
    _Assign(_Ptr, len);
    return *this;
}

template<class CharT>
inline StringRef<CharT>& StringRef<CharT>::assign(const StringRef<CharT>& _Str,
    size_type off, size_type count)
{
    size_type len = _Str.length();
    if (off > len) {
        return *this;
    }
    if (off + count > len) {
        count = len - off;
    }
    _Assign(_Str.str_ + off, count);
    return *this;
}

template<class CharT>
inline StringRef<CharT>& StringRef<CharT>::assign(const StringRef<CharT>& _Str)
{
    *this = _Str;
    return *this;
}

template<class CharT>
inline StringRef<CharT>& StringRef<CharT>::assign(size_type count, value_type _Ch)
{
    if (count == 0) {
        free();
        return *this;
    }

    if (header()->refCount > 1 || count > capacity()) {
        free();
        Allocate(count);
    }

    _set(str_, _Ch, count);
    return *this;
}

template<class CharT>
inline StringRef<CharT>& StringRef<CharT>::assign(const_iterator _First, const_iterator _Last)
{
    assign(_First, (size_type)(_Last - _First));
    return *this;
}

// =============================================================================

template<class T>
inline StringRef<T>& StringRef<T>::replace(size_type pos, size_type count, const_str strNew)
{
    return replace(pos, count, strNew, strlen(strNew));
}

template<class T>
inline StringRef<T>& StringRef<T>::replace(size_type pos, size_type count, const_str strNew, size_type count2)
{
    erase(pos, count);
    insert(pos, strNew, count2);
    return *this;
}

template<class T>
inline StringRef<T>& StringRef<T>::replace(size_type pos, size_type count, size_type nNumChars, value_type chNew)
{
    erase(pos, count);
    insert(pos, nNumChars, chNew);
    return *this;
}

template<class T>
inline StringRef<T>& StringRef<T>::replace(value_type chOld, value_type chNew)
{
    if (chOld != chNew) {
        makeUnique();
        value_type* strend = str_ + length();
        for (value_type* str = str_; str != strend; ++str) {
            if (*str == chOld) {
                *str = chNew;
            }
        }
    }
    return *this;
}

template<class T>
inline StringRef<T>& StringRef<T>::replace(const_str strOld, const_str strNew)
{
    size_type sourceLen = strlen(strOld);

    if (sourceLen == 0) {
        return *this;
    }

    const_str strOldStart = strOld;
    const_str strOldEnd = strOld + sourceLen;

    size_type replacementLen = strlen(strNew);

    size_type count = 0;
    value_type* strStart = str_;
    value_type* strEnd = str_ + length();
    value_type* strTarget;
    while (strStart < strEnd) {
        while ((strTarget = const_cast<value_type*>(strUtil::Find(strStart, strEnd, strOldStart, strOldEnd))) != nullptr) {
            count++;
            strStart = strTarget + sourceLen;
        }
        strStart += strlen(strStart) + 1;
    }

    if (count > 0) {
        makeUnique();

        size_type nOldLength = length();
        size_type nNewLength = nOldLength + (replacementLen - sourceLen) * count;
        if (capacity() < nNewLength || header()->refCount > 1) {
            XStrHeader* pOldData = header();
            const_str pstr = str_;
            Allocate(nNewLength);
            _copy(str_, pstr, pOldData->length);
            freeData(pOldData);
        }
        strStart = str_;
        strEnd = str_ + length();

        while (strStart < strEnd) {
            while ((strTarget = const_cast<value_type*>(strUtil::Find(strStart, strEnd, strOldStart, strOldEnd))) != nullptr) {
                size_type nBalance = nOldLength - ((size_type)(strTarget - str_) + sourceLen);
                _move(strTarget + replacementLen, strTarget + sourceLen, nBalance);
                _copy(strTarget, strNew, replacementLen);
                strStart = strTarget + replacementLen;
                strStart[nBalance] = 0;
                nOldLength += (replacementLen - sourceLen);
            }
            strStart += strlen(strStart) + 1;
        }
        header()->length = (int)nNewLength;
    }

    return *this;
}

// =============================================================================

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::insert(size_type nIndex, value_type ch)
{
    return insert(nIndex, 1, ch);
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::insert(size_type nIndex, size_type count, value_type ch)
{
    makeUnique();

    size_type nNewLength = length();
    if (nIndex > nNewLength) {
        nIndex = nNewLength;
    }

    nNewLength += count;

    if (capacity() < nNewLength) {
        XStrHeader* pOldData = header();
        const_str pstr = str_;
        Allocate(nNewLength);
        _copy(str_, pstr, pOldData->length + 1);
        freeData(pOldData);
    }

    _move(str_ + nIndex + count, str_ + nIndex, (nNewLength - nIndex));
    _set(str_ + nIndex, ch, count);
    header()->length = safe_static_cast<int, size_type>(nNewLength);

    return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::insert(size_type nIndex, const_str pstr)
{
    return insert(nIndex, pstr, strlen(pstr));
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::insert(size_type nIndex, const_str pstr, size_type count)
{
    size_type nInsertLength = count;
    size_type nNewLength = length();
    if (nInsertLength > 0) {
        makeUnique();
        if (nIndex > nNewLength) {
            nIndex = nNewLength;
        }
        nNewLength += nInsertLength;

        if (capacity() < nNewLength) {
            XStrHeader* pOldData = header();
            const_str pOldStr = str_;
            Allocate(nNewLength);
            _copy(str_, pOldStr, (pOldData->length + 1));
            freeData(pOldData);
        }

        _move(str_ + nIndex + nInsertLength, str_ + nIndex, (nNewLength - nIndex - nInsertLength + 1));
        _copy(str_ + nIndex, pstr, nInsertLength);
        header()->length = safe_static_cast<int, size_type>(nNewLength);
        str_[length()] = 0;
    }
    return *this;
}

// =============================================================================

//! delete count characters starting at zero-based index
template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::erase(size_type nIndex, size_type count)
{
    if (count < 0 || count > length() - nIndex) {
        count = length() - nIndex;
    }

    if (count > 0 && nIndex < length()) {
        makeUnique();
        size_type nNumToCopy = length() - (nIndex + count) + 1;
        _move(str_ + nIndex, str_ + nIndex + count, nNumToCopy);
        header()->length = safe_static_cast<int, size_t>(length() - count);
    }

    return *this;
}

// =============================================================================

template<typename CharT>
bool StringRef<CharT>::compare(const StrT& Str) const
{
    return core::strUtil::IsEqual(begin(), Str.begin());
}

template<typename CharT>
bool StringRef<CharT>::compare(const_str ptr) const
{
    X_ASSERT_NOT_NULL(ptr);
    return core::strUtil::IsEqual(begin(), end(), ptr);
}

template<typename CharT>
int StringRef<CharT>::compareInt(const StrT& _Str) const
{
    return strcmp(str_, _Str.str_);
}

template<typename CharT>
int StringRef<CharT>::compareInt(const_str ptr) const
{
    X_ASSERT_NOT_NULL(ptr);
    return strcmp(str_, ptr);
}

// =============================================================================

template<typename CharT>
bool StringRef<CharT>::compareCaseInsen(const StrT& Str) const
{
    return core::strUtil::IsEqualCaseInsen(begin(), end(), Str.begin(), Str.end());
}

template<typename CharT>
bool StringRef<CharT>::compareCaseInsen(const_str ptr) const
{
    return core::strUtil::IsEqualCaseInsen(begin(), end(), ptr);
}

// =============================================================================

template<typename CharT>
typename StringRef<CharT>::const_str StringRef<CharT>::find(value_type ch) const
{
    return strUtil::Find(begin(), end(), ch);
}

template<typename CharT>
typename StringRef<CharT>::const_str StringRef<CharT>::find(const_str subStr) const
{
    return strUtil::Find(begin(), end(), subStr);
}

// =============================================================================

template<typename CharT>
void StringRef<CharT>::swap(StrT& oth)
{
    core::Swap(str_, oth.str_);
}

// =============================================================================

template<typename CharT>
StringRef<CharT> StringRef<CharT>::substr(const_str pBegin, const_str pEnd) const
{
    if (pBegin > end())
        return StringRef<CharT>();

    if (pBegin == nullptr || pBegin < begin())
        pBegin = begin();

    if (pEnd == nullptr || pEnd > end())
        pEnd = end();

    return StringRef<CharT>(pBegin, pEnd);
}

// =============================================================================

template<class CharT>
inline StringRef<CharT> StringRef<CharT>::right(size_type count) const
{
    //	if (count == npos)
    //		return StringRef<CharT>();
    //	else
    if (count > length()) {
        return *this;
    }

    return StringRef<CharT>(str_ + length() - count, count);
}

template<class CharT>
inline StringRef<CharT> StringRef<CharT>::left(size_type count) const
{
    //	if (count == npos)
    //		return StringRef<CharT>();
    //	else
    if (count > length()) {
        count = length();
    }

    return StringRef<CharT>(str_, count);
}

// =============================================================================

template<class CharT>
inline void StringRef<CharT>::_copy(value_type* dest, const value_type* src, size_type count)
{
    if (dest != src) {
        memcpy(dest, src, count * sizeof(value_type));
    }
}

template<class CharT>
inline void StringRef<CharT>::_move(value_type* dest, const value_type* src, size_type count)
{
    memmove(dest, src, count * sizeof(value_type));
}

template<class CharT>
inline void StringRef<CharT>::_set(value_type* dest, value_type ch, size_type count)
{
    memset(dest, ch, count * sizeof(value_type));
}

template<>
inline void StringRef<wchar_t>::_set(value_type* dest, value_type ch, size_type count)
{
    wmemset(dest, ch, count);
}

// =============================================================================

template<class CharT>
typename StringRef<CharT>::XStrHeader* StringRef<CharT>::header(void) const
{
    X_ASSERT_NOT_NULL(str_);

    return (reinterpret_cast<XStrHeader*>(str_) - 1);
}

// dose not check current length
template<class CharT>
void StringRef<CharT>::Allocate(size_type length)
{
    X_ASSERT(length >= 0 && length <= (INT_MAX - 1), "length is invalid")
    (length);

    if (length == 0) {
        SetEmpty();
    }
    else {
        X_ASSERT_NOT_NULL(gEnv->pStrArena);

        size_type allocLen = sizeof(XStrHeader) + ((length + 1) * sizeof(value_type));

        XStrHeader* pData = reinterpret_cast<XStrHeader*>(X_NEW_ARRAY_OFFSET(BYTE, allocLen,
            gEnv->pStrArena, "StringBuf", sizeof(XStrHeader)));

        pData->refCount = 1;
        str_ = pData->getChars();
        pData->length = safe_static_cast<int, size_type>(length);
        pData->allocSize = safe_static_cast<int, size_type>(length);
        str_[length] = 0; // null terminated string.
    }
}

template<class CharT>
void StringRef<CharT>::free(void)
{
    if (header()->refCount >= 0) // Not empty string.
    {
        freeData(header());
        SetEmpty();
    }
}

template<class CharT>
void StringRef<CharT>::SetEmpty(void)
{
    str_ = emptyHeader()->getChars();
}

template<class CharT>
void StringRef<CharT>::makeUnique(void)
{
    if (header()->refCount > 1) {
        // If string is shared, make a copy of string buffer.
        XStrHeader* pOldData = header();
        // This will not free header because reference count is greater then 1.
        free();
        // Allocate a new string buffer.
        Allocate(pOldData->length);
        // Full copy of null terminated string.
        _copy(str_, pOldData->getChars(), pOldData->length + 1);
    }
}

template<class CharT>
void StringRef<CharT>::Concatenate(const_str sStr1, size_type nLen1, const_str sStr2, size_type nLen2)
{
    size_type nLen = nLen1 + nLen2;

    if (nLen1 * 2 > nLen) {
        nLen = nLen1 * 2;
    }

    if (nLen != 0) {
        if (nLen < 8) {
            nLen = 8;
        }

        Allocate(nLen);
        _copy(str_, sStr1, nLen1);
        _copy(str_ + nLen1, sStr2, nLen2);
        check_convert(header()->length) = nLen1 + nLen2;
        str_[nLen1 + nLen2] = 0;
    }
}

template<class CharT>
void StringRef<CharT>::ConcatenateInPlace(const_str sStr, size_type nLen)
{
    if (nLen != 0) {
        // Check if this string is shared (reference count greater then 1)
        // or not enough capacity to store new string.
        // Then allocate new string buffer.
        if (header()->refCount > 1 || length() + nLen > capacity()) {
            XStrHeader* pOldData = header();
            Concatenate(str_, length(), sStr, nLen);
            freeData(pOldData);
        }
        else {
            _copy(str_ + length(), sStr, nLen);
            check_convert(header()->length) = header()->length + nLen;
            str_[header()->length] = 0; // Make null terminated string.
        }
    }
}

template<class CharT>
void StringRef<CharT>::_Assign(const_str sStr, size_type Len)
{
    // Check if this string is shared (reference count greater then 1)
    // or not enough capacity to store new string.
    // Then allocate new string buffer.
    if (header()->refCount > 1 || Len > capacity()) {
        free();
        Allocate(Len);
    }
    // Copy characters from new string to this buffer.
    _copy(str_, sStr, Len);
    // Set new length.
    header()->length = safe_static_cast<int, size_t>(Len);
    // Make null terminated string.
    str_[Len] = 0;
}

// =============================================================================

template<class CharT>
void StringRef<CharT>::freeData(XStrHeader* pData)
{
    if (pData->refCount >= 0) // Not empty string.
    {
        X_ASSERT(pData->refCount != 0, "invalid ref count")
        (pData->refCount);
        if (pData->release() <= 0) {
            X_ASSERT_NOT_NULL(gEnv);
            X_ASSERT_NOT_NULL(gEnv->pStrArena);

            X_DELETE(pData, gEnv->pStrArena);
            //	delete[] pData;
        }
    }
}

template<class CharT>
size_t StringRef<CharT>::strlen(const_str pStr)
{
    return core::strUtil::strlen(pStr);
}

// =============================================================================

template<class T>
inline bool operator==(const StringRef<T>& s1, const StringRef<T>& s2)
{
    return s1.compare(s2);
}

template<class T>
inline bool operator==(const StringRef<T>& s1, const typename StringRef<T>::value_type* s2)
{
    return s1.compare(s2);
}

template<class T>
inline bool operator==(const typename StringRef<T>::value_type* s1, const StringRef<T>& s2)
{
    return s2.compare(s1);
}

template<class T>
inline bool operator!=(const StringRef<T>& s1, const StringRef<T>& s2)
{
    return !s1.compare(s2);
}

template<class T>
inline bool operator!=(const StringRef<T>& s1, const typename StringRef<T>::value_type* s2)
{
    return !s1.compare(s2);
}

template<class T>
inline bool operator!=(const typename StringRef<T>::value_type* s1, const StringRef<T>& s2)
{
    return !s2.compare(s1);
}

//! compare helpers

template<class T>
inline bool operator<(const StringRef<T>& s1, const StringRef<T>& s2)
{
    return s1.compareInt(s2) < 0;
}

template<class T>
inline bool operator<(const StringRef<T>& s1, const typename StringRef<T>::value_type* s2)
{
    return s1.compareInt(s2) < 0;
}

template<class T>
inline bool operator<(const typename StringRef<T>::value_type* s1, const StringRef<T>& s2)
{
    return s2.compareInt(s1) > 0;
}

template<class T>
inline bool operator>(const StringRef<T>& s1, const StringRef<T>& s2)
{
    return s1.compareInt(s2) > 0;
}

template<class T>
inline bool operator>(const StringRef<T>& s1, const typename StringRef<T>::value_type* s2)
{
    return s1.compareInt(s2) > 0;
}

template<class T>
inline bool operator>(const typename StringRef<T>::value_type* s1, const StringRef<T>& s2)
{
    return s2.compareInt(s1) < 0;
}

template<class T>
inline bool operator<=(const StringRef<T>& s1, const StringRef<T>& s2)
{
    return s1.compareInt(s2) <= 0;
}

template<class T>
inline bool operator<=(const StringRef<T>& s1, const typename StringRef<T>::value_type* s2)
{
    return s1.compareInt(s2) <= 0;
}

template<class T>
inline bool operator<=(const typename StringRef<T>::value_type* s1, const StringRef<T>& s2)
{
    return s2.compareInt(s1) >= 0;
}

template<class T>
inline bool operator>=(const StringRef<T>& s1, const StringRef<T>& s2)
{
    return s1.compareInt(s2) >= 0;
}

template<class T>
inline bool operator>=(const StringRef<T>& s1, const typename StringRef<T>::value_type* s2)
{
    return s1.compareInt(s2) >= 0;
}

template<class T>
inline bool operator>=(const typename StringRef<T>::value_type* s1, const StringRef<T>& s2)
{
    return s2.compareInt(s1) <= 0;
}

// =============================================================================

template<class T>
inline StringRef<T> operator+(const StringRef<T>& string1, typename StringRef<T>::value_type ch)
{
    StringRef<T> s(string1);
    s.append(1, ch);
    return s;
}

template<class T>
inline StringRef<T> operator+(typename StringRef<T>::value_type ch, const StringRef<T>& str)
{
    StringRef<T> s;
    s.reserve(str.size() + 1);
    s.append(1, ch);
    s.append(str);
    return s;
}

template<class T>
inline StringRef<T> operator+(const StringRef<T>& string1, const StringRef<T>& string2)
{
    StringRef<T> s(string1);
    s.append(string2);
    return s;
}

template<class T>
inline StringRef<T> operator+(const StringRef<T>& str1, const typename StringRef<T>::value_type* str2)
{
    StringRef<T> s(str1);
    s.append(str2);
    return s;
}

template<class T>
inline StringRef<T> operator+(const typename StringRef<T>::value_type* str1, const StringRef<T>& str2)
{
    X_ASSERT_NOT_NULL(str1);

    StringRef<T> s;
    s.reserve(core::strUtil::strlen(str1) + str2.size());
    s.append(str1);
    s.append(str2);
    return s;
}
