
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(void) :
    len_(0)
{
    str_[0] = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const StackString<N, TChar>& oth) :
    len_(oth.len_)
{
    memcpy(str_, oth.str_, (len_ * sizeof(TChar)) + 1);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const TChar* const str) :
    len_(strUtil::strlen(str))
{
    X_ASSERT(len_ < N, "String(%d) \"%s\" does not fit into StackString of size %d.", len_, str, N)(len_, N);
    memcpy(str_, str, len_ + 1);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const wchar_t* const str) :
    len_(strUtil::strlen(str))
{
    X_ASSERT(len_ < N, "String(%d) \"%s\" does not fit into StackString of size %d.", len_, str, N)(len_, N);
    strUtil::Convert(str, str_, capacity());
    str_[len_] = L'\0';
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const wchar_t* const beginInclusive, const wchar_t* const endExclusive) :
    len_(safe_static_cast<size_t>(endExclusive - beginInclusive))
{
    X_ASSERT(len_ < N, "String(%d) \"%s.*\" does not fit into StackString of size %d.", len_, len_, beginInclusive, N)(len_, N);
    strUtil::Convert(beginInclusive, str_, capacity());
    str_[len_] = L'\0';
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const StringRange<TChar>& range) :
    len_(safe_static_cast<size_t>(range.getLength()))
{
    X_ASSERT(len_ < N, "StringRange does not fit into StackString of size %d.", N)();

    // ranges do not necessarily contain a null-terminator, hence we add it manually
    memcpy(str_, range.getStart(), len_);
    str_[len_] = '\0';
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const TChar* const beginInclusive, const TChar* const endExclusive) :
    len_(safe_static_cast<size_t>(endExclusive - beginInclusive))
{
    X_ASSERT(len_ < N, "String of length %d does not fit into StackString of size %d.", len_, N)(len_, N);
    memcpy(str_, beginInclusive, len_);
    str_[len_] = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const bool b) :
    len_(1)
{
    X_ASSERT(len_ < N, "bool val does not fit into stackstring of size %d.", len_)(len_, N);

    str_[0] = b ? '1' : '0';
    str_[1] = '\0';
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const char c) :
    len_(1)
{
    X_ASSERT(len_ < N, "TChar val does not fit into stackstring of size %d.", len_)(len_, N);

    str_[0] = static_cast<TChar>(c);
    str_[1] = '\0';
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const wchar_t c) :
    len_(1)
{
    X_ASSERT(len_ < N, "TChar val does not fit into stackstring of size %d.", len_)(len_, N);

    str_[0] = static_cast<TChar>(c);
    str_[1] = '\0';
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const int i) :
    len_(12)
{
    X_ASSERT(len_ < N, "int val does not fit into stackstring of size %d.", len_)(len_, N);

    len_ = sprintf_s(str_, "%" PRIi32, i);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const unsigned u) :
    len_(12)
{
    X_ASSERT(len_ < N, "unsigned val does not fit into stackstring of size %d.", len_)(len_, N);

    len_ = sprintf_s(str_, "%" PRIu32, u);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const float f) :
    len_(24) // represent any float.
{
    X_ASSERT(len_ < N, "unsigned val does not fit into stackstring of size %d.", len_)(len_, N);

    TChar text[64];

    size_t l = sprintf_s(text, "%f", f);

    while (l > 0 && text[l - 1] == '0') {
        text[--l] = '\0';
    }
    while (l > 0 && text[l - 1] == '.') {
        text[--l] = '\0';
    }

    strcpy_s(str_, text);
    len_ = l;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const unsigned __int64 u) :
    len_(24)
{
    X_ASSERT(len_ < N, "unsigned __int64 does not fit into stackstring of size %d.", len_)(len_, N);

    len_ = sprintf_s(str_, "%" PRIu64, u);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
StackString<N, TChar>::StackString(const __int64 u) :
    len_(24)
{
    X_ASSERT(len_ < N, "__int64 does not fit into stackstring of size %d.", len_)(len_, N);

    len_ = sprintf_s(str_, "%" PRIi64, u);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
void StackString<N, TChar>::append(TChar ch, size_t count)
{
    memset(str_ + len_, ch, count);
    len_ += count;
    str_[len_] = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
void StackString<N, TChar>::append(const TChar* str)
{
    append(str, strUtil::strlen(str));
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
void StackString<N, TChar>::append(const TChar* str, size_t count)
{
    X_ASSERT(len_ + count < N, "Cannot append %d character(s) from string \"%s\". Not enough space left.", count, str)(len_, N);
    memcpy(str_ + len_, str, count);
    len_ += count;
    str_[len_] = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
void StackString<N, TChar>::append(const TChar* str, const TChar* end)
{
    append(str, safe_static_cast<size_t>(end - str));
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
void StackString<N, TChar>::appendFmt(const TChar* format, ...)
{
    va_list args;
    va_start(args, format);

    const int charactersWritten = _vsnprintf_s(str_ + len_, N - len_, _TRUNCATE, format, args);
    if (charactersWritten < 0) {
        X_WARNING("StackString", "String truncation occurred during append operation.");
        len_ = N - 1;
        str_[len_] = 0;
    }
    else {
        len_ += charactersWritten;
    }

    va_end(args);
}

template<size_t N, typename TChar>
void StackString<N, TChar>::appendFmt(const TChar* format, va_list args)
{
    const int charactersWritten = _vsnprintf_s(str_ + len_, N - len_, _TRUNCATE, format, args);
    if (charactersWritten < 0) {
        X_WARNING("StackString", "String truncation occurred during append operation.");
        len_ = N - 1;
        str_[len_] = 0;
    }
    else {
        len_ += charactersWritten;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
void StackString<N, TChar>::set(const TChar* str)
{
    size_t len = core::Min<size_t>(N - 1, strUtil::strlen(str));

    X_ASSERT(len < N, "String of length %d does not fit into StackString of size %d.", len, N)();

    memcpy(str_, str, len);
    len_ = len;
    str_[len_] = 0;
}

template<size_t N, typename TChar>
void StackString<N, TChar>::set(const TChar* const beginInclusive, const TChar* const endExclusive)
{
    size_t len = core::Min<size_t>(N - 1, (endExclusive - beginInclusive));

    X_ASSERT(len < N, "String of length %d does not fit into StackString of size %d.", len, N)();

    memcpy(str_, beginInclusive, len);
    len_ = len;
    str_[len_] = 0;
}

template<size_t N, typename TChar>
void StackString<N, TChar>::set(const StringRange<TChar>& range)
{
    X_ASSERT(range.getLength() < N, "String of length %d does not fit into StackString of size %d.", range.getLength(), N)();

    memcpy(str_, range.begin(), range.getLength());
    len_ = range.getLength();
    str_[len_] = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<size_t N, typename TChar>
void StackString<N, TChar>::setFmt(const TChar* format, ...)
{
    str_[0] = '\0';
    len_ = 0;

    va_list args;
    va_start(args, format);

    const int charactersWritten = _vsnprintf_s(str_, N, _TRUNCATE, format, args);
    if (charactersWritten < 0) {
        X_WARNING("StackString", "String truncation occurred during append operation.");
        len_ = N - 1;
        str_[len_] = 0;
    }
    else {
        len_ += charactersWritten;
    }

    va_end(args);
}

template<size_t N, typename TChar>
void StackString<N, TChar>::setFmt(const TChar* format, va_list args)
{
    str_[0] = '\0';
    len_ = 0;

    const int charactersWritten = _vsnprintf_s(str_, N, _TRUNCATE, format, args);
    if (charactersWritten < 0) {
        X_WARNING("StackString", "String truncation occurred during append operation.");
        len_ = N - 1;
        str_[len_] = 0;
    }
    else {
        len_ += charactersWritten;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

template<size_t N, typename TChar>
bool StackString<N, TChar>::replace(const TChar* start, const TChar* original, const TChar* replacement)
{
    X_ASSERT(strcmp(original, replacement) != 0, "Replace operation cannot be performed. Strings are identical.")(original, replacement);
    X_ASSERT(start >= begin() && start <= end(), "start dose not point to a section of the string")(start, original, replacement);

    // find the position of the string to replace
    const size_t originalLength = strUtil::strlen(original);
    const TChar* pos = strUtil::Find(start, str_ + len_, original, originalLength);
    if (!pos) {
        return false;
    }

    TChar* const replacePos = const_cast<TChar*>(pos);

    // adjust the length of our string, assuming that the string has been replaced
    const size_t replacementLength = strUtil::strlen(replacement);
    const size_t newLength = len_ + replacementLength - originalLength;
    X_ASSERT(newLength < N, "Cannot replace \"%s\" with \"%s\" in string \"%s\". Not enough space left.", original, replacement, str_)(len_, newLength, N);

    // move characters so that the replacement fits in-between
    const size_t toCopy = safe_static_cast<size_t>((str_ + len_) - (replacePos + originalLength));
    memmove(replacePos + replacementLength, replacePos + originalLength, toCopy);

    // replace characters
    memcpy(replacePos, replacement, replacementLength);

    str_[newLength] = 0;
    len_ = newLength;

    return true;
}

template<size_t N, typename TChar>
bool StackString<N, TChar>::replace(const TChar* original, const TChar* replacement)
{
    // find the position of the string to replace
    const size_t originalLength = strUtil::strlen(original);

    if (originalLength == 0) {
        return true;
    }

    X_ASSERT(strcmp(original, replacement) != 0, "Replace operation cannot be performed. Strings are identical.")(original, replacement);

    const TChar* pos = strUtil::Find(str_, str_ + len_, original, originalLength);
    if (!pos) {
        return false;
    }

    TChar* const replacePos = const_cast<TChar*>(pos);

    // adjust the length of our string, assuming that the string has been replaced
    const size_t replacementLength = strUtil::strlen(replacement);
    const size_t newLength = len_ + replacementLength - originalLength;
    X_ASSERT(newLength < N, "Cannot replace \"%s\" with \"%s\" in string \"%s\". Not enough space left.", original, replacement, str_)(len_, newLength, N);

    // move characters so that the replacement fits in-between
    const size_t toCopy = safe_static_cast<size_t>((str_ + len_) - (replacePos + originalLength));
    memmove(replacePos + replacementLength, replacePos + originalLength, toCopy);

    // replace characters
    memcpy(replacePos, replacement, replacementLength);

    str_[newLength] = 0;
    len_ = newLength;

    return true;
}

template<size_t N, typename TChar>
bool StackString<N, TChar>::replace(const TChar original, const TChar replacement)
{
    // find me baby
    TChar* start = str_;
    while (start < end()) {
        if (*start == original) {
            *start = replacement;
            return true;
        }
        ++start;
    }
    return false;
}

template<size_t N, typename TChar>
size_t StackString<N, TChar>::replaceAll(const TChar* original, const TChar* replacement)
{
    for (size_t count = 0;; ++count) {
        if (!replace(original, replacement)) {
            return count;
        }
    }
}

/// Replaces all occurrences of a character, and returns the number of occurrences replaced.
template<size_t N, typename TChar>
size_t StackString<N, TChar>::replaceAll(const TChar original, const TChar replacement)
{
    // find me baby
    size_t count = 0;
    TChar* start = str_;
    while (start < end()) {
        if (*start == original) {
            *start = replacement;
            count++;
        }
        ++start;
    }
    return count;
}

template<size_t N, typename TChar>
void StackString<N, TChar>::trimWhitespace(void)
{
    const TChar* pos = strUtil::FindNonWhitespace(str_, str_ + len_);

    // only white space.
    if (!pos) {
        len_ = 0;
        str_[0] = 0;
        return;
    }

    // string contains at least one non-whitespace character
    if (pos > str_) {
        // there are whitespaces to the left, trim them
        memmove(str_, pos, len_);
        len_ -= safe_static_cast<size_t>(pos - str_);
    }

    pos = strUtil::FindLastNonWhitespace(str_, str_ + len_);
    if (pos && pos < str_ + len_ - 1) {
        // there are whitespaces to the right, trim them
        len_ = safe_static_cast<size_t>(pos - str_ + 1);
        str_[len_] = 0;
    }
}

template<size_t N, typename TChar>
void StackString<N, TChar>::trim(TChar character)
{
    trimLeft(character);
    trimRight(character);
}

template<size_t N, typename TChar>
void StackString<N, TChar>::stripTrailing(const TChar c)
{
    size_t i;

    for (i = len_; i > 0 && str_[i - 1] == c; i--) {
        str_[i - 1] = '\0';
        len_--;
    }
}

template<size_t N, typename TChar>
void StackString<N, TChar>::stripColorCodes(void)
{
    // when we find color codes we wnat to just shift remaning chars.
    StackString<N, TChar> buf(*this);

    clear();

    const size_t len = buf.length();
    for (size_t i = 0; i < len; i++) {
        const char& cur = buf.str_[i];
        if (cur == '^' && (i + 1) < len && core::strUtil::IsDigit(buf.str_[i + 1])) {
            i += 1;
        }
        else {
            str_[len_++] = cur;
        }
    }

    str_[len_] = '\0';
}

template<size_t N, typename TChar>
void StackString<N, TChar>::trimLeft(const TChar* pos)
{
    auto trimLength = safe_static_cast<size_t>(pos - str_);

    memmove(str_, pos, (len_ - trimLength) + 1);
    len_ -= trimLength;
}

template<size_t N, typename TChar>
void StackString<N, TChar>::trimLeft(TChar ch)
{
    const char* pCur = begin();

    while (pCur < end() && *pCur == ch) {
        ++pCur;
    }

    if (pCur != begin()) {
        trimLeft(pCur);
    }
}

template<size_t N, typename TChar>
void StackString<N, TChar>::trimRight(const TChar* pos)
{
    len_ = safe_static_cast<size_t>(pos - str_);
    str_[len_] = 0;
}

template<size_t N, typename TChar>
void StackString<N, TChar>::trimRight(TChar ch)
{
    const char* pCur = end() - 1;

    while (pCur >= begin() && *pCur == ch) {
        --pCur;
    }

    if (pCur != end() - 1) {
        trimRight(pCur + 1);
    }
}

template<size_t N, typename TChar>
StackString<N, TChar>& StackString<N, TChar>::trim(void)
{
    return trimRight().trimLeft();
}

template<size_t N, typename TChar>
StackString<N, TChar>& StackString<N, TChar>::trimLeft(void)
{
    // we just loop over the string while there is white space and we are inside the string.
    const TChar* str = str_;

    while (*str && strUtil::IsWhitespace((TChar)*str)) {
        str++;
    }

    // if they not equal we found white space.
    if (str != str_) {
        size_t Off = (size_t)(str - str_);
        size_t NewLength = this->length() - Off;

        memmove(str_, str_ + Off, NewLength);

        str_[NewLength] = '\0';

        len_ = NewLength;
    }

    return *this;
}

// removes any leading white space chars.
template<size_t N, typename TChar>
StackString<N, TChar>& StackString<N, TChar>::trimRight(void)
{
    // we want to start at the end and continue untill no more whitespace or end :)
    const TChar* str = this->end() - 1;
    const TChar* start = this->begin();

    while (str > start && strUtil::IsWhitespace((TChar)*str)) {
        --str;
    }

    if (str != end()) {
        // trim me baby
        size_t NewLength = (size_t)(str - str_) + 1;
        str_[NewLength] = 0;

        len_ = NewLength;
    }

    return *this;
}

template<size_t N, typename TChar>
void StackString<N, TChar>::clear(void)
{
    len_ = 0;
    str_[0] = 0;
}

template<size_t N, typename TChar>
inline bool StackString<N, TChar>::isEqual(const TChar* other) const
{
    return strUtil::IsEqual(str_, str_ + len_, other, other + strlen(other));
}

template<size_t N, typename TChar>
inline const TChar* StackString<N, TChar>::find(TChar ch) const
{
    return strUtil::Find(str_, str_ + len_, ch);
}

template<size_t N, typename TChar>
inline const TChar* StackString<N, TChar>::findLast(TChar ch) const
{
    return strUtil::FindLast(str_, str_ + len_, ch);
}

template<size_t N, typename TChar>
inline const TChar* StackString<N, TChar>::find(const TChar* string) const
{
    return strUtil::Find(str_, str_ + len_, string);
}

template<size_t N, typename TChar>
inline const TChar* StackString<N, TChar>::findCaseInsen(TChar ch) const
{
    return strUtil::FindCaseInsensitive(str_, str_ + len_, ch);
}

template<size_t N, typename TChar>
inline const TChar* StackString<N, TChar>::findCaseInsen(const TChar* string) const
{
    return strUtil::FindCaseInsensitive(str_, str_ + len_, string);
}

template<size_t N, typename TChar>
inline const TChar* StackString<N, TChar>::findCaseInsen(const TChar* pBegin, const TChar* pEnd) const
{
    return strUtil::FindCaseInsensitive(str_, str_ + len_, pBegin, pEnd);
}

template<size_t N, typename TChar>
inline bool StackString<N, TChar>::operator==(const StackString& oth) const
{
    return isEqual(oth.c_str());
}

template<size_t N, typename TChar>
inline bool StackString<N, TChar>::operator!=(const StackString& oth) const
{
    return !isEqual(oth.c_str());
}

template<size_t N, typename TChar>
inline StackString<N, TChar>& StackString<N, TChar>::operator=(const StackString& oth)
{
    memcpy(str_, oth.str_, oth.len_);
    len_ = oth.len_;
    str_[len_] = 0;
    return *this;
}

template<size_t N, typename TChar>
inline TChar& StackString<N, TChar>::operator[](size_t i)
{
    // allow access to the null terminator
    X_ASSERT(i <= len_, "Character index %d cannot be accessed. Subscript out of range.", i)(N, str_, len_);
    return str_[i];
}

template<size_t N, typename TChar>
inline const TChar& StackString<N, TChar>::operator[](size_t i) const
{
    // allow access to the null terminator
    X_ASSERT(i <= len_, "Character index %d cannot be accessed. Subscript out of range.", i)(N, str_, len_);
    return str_[i];
}

template<size_t N, typename TChar>
inline const TChar* StackString<N, TChar>::c_str(void) const
{
    return str_;
}

template<size_t N, typename TChar>
inline const TChar* StackString<N, TChar>::data(void) const
{
    return str_;
}

template<size_t N, typename TChar>
inline size_t StackString<N, TChar>::length(void) const
{
    return len_;
}

template<size_t N, typename TChar>
inline bool StackString<N, TChar>::isEmpty(void) const
{
    return len_ == 0;
}

template<size_t N, typename TChar>
inline bool StackString<N, TChar>::isNotEmpty(void) const
{
    return len_ > 0;
}

template<size_t N, typename TChar>
inline void StackString<N, TChar>::toLower(void)
{
    for (size_t i = 0; i < len_; ++i) {
        str_[i] = safe_static_cast<TChar>(tolower(str_[i]));
    }
}

template<size_t N, typename TChar>
inline void StackString<N, TChar>::toUpper(void)
{
    for (size_t i = 0; i < len_; ++i) {
        str_[i] = safe_static_cast<TChar>(toupper(str_[i]));
    }
}

template<size_t N, typename TChar>
inline const TChar* StackString<N, TChar>::begin(void) const
{
    return str_;
}

template<size_t N, typename TChar>
inline const TChar* StackString<N, TChar>::end(void) const
{
    return str_ + this->len_;
}

template<size_t N, typename TChar>
inline constexpr size_t StackString<N, TChar>::capacity(void) const
{
    return N;
}

template<size_t N, typename TChar>
inline size_t StackString<N, TChar>::freeSpace(void) const
{
    return capacity() - len_;
}
