
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N, wchar_t>::StackString(void)
	: len_(0)
{
	str_[0] = 0;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N, wchar_t>::StackString(const wchar_t* const str)
	: len_(strUtil::strlen(str))
{
	X_ASSERT(len_ < N, "String(%d) \"%s\" does not fit into StackString of size %d.", len_, str, N)();
	memcpy(str_, str, (len_ + 1) * sizeof(wchar_t));
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N, wchar_t>::StackString(const char* const str)
	: len_(strUtil::strlen(str))
{
	str_[0] = 0;
	X_ASSERT_UNREACHABLE();
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------


template <size_t N>
StackString<N, wchar_t>::StackString(const StringRange<wchar_t>& range)
	: len_(safe_static_cast<size_t>(range.GetLength()))
{
	X_ASSERT(len_ < N, "StringRange does not fit into StackString of size %d.", N)();

	// ranges do not necessarily contain a null-terminator, hence we add it manually
	memcpy(str_, range.GetStart(), len_* sizeof(wchar_t));
	str_[len_] = L'\0';
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N, wchar_t>::StackString(const wchar_t* const beginInclusive, const wchar_t* const endExclusive)
	: len_(safe_static_cast<size_t>(endExclusive - beginInclusive))
{
	X_ASSERT(len_ < N, "String of length %d does not fit into StackString of size %d.", len_, N)();
	memcpy(str_, beginInclusive, len_ * sizeof(wchar_t));
	str_[len_] = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N, wchar_t>::StackString(const bool b)
	: len_(1)
{
	X_ASSERT(len_ < N, "bool val does not fit into stackstring of size %d.", len_, N)();

	str_[0] = b ? L'1' : L'0';
	str_[1] = L'\0';
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N, wchar_t>::StackString(const char c)
	: len_(1)
{
	X_ASSERT(len_ < N, "TChar val does not fit into stackstring of size %d.", len_, N)();

	str_[0] = static_cast<wchar_t>(c);
	str_[1] = '\0';
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N, wchar_t>::StackString(const wchar_t c)
	: len_(1)
{
	X_ASSERT(len_ < N, "TChar val does not fit into stackstring of size %d.", len_, N)();

	str_[0] = c;
	str_[1] = '\0';
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N, wchar_t>::StackString(const int i)
	: len_(12)
{
	X_ASSERT(len_ < N, "int val does not fit into stackstring of size %d.", len_, N)();

	len_ = swprintf_s(str_, L"%d", i);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N, wchar_t>::StackString(const unsigned u)
	: len_(12)
{
	X_ASSERT(len_ < N, "unsigned val does not fit into stackstring of size %d.", len_, N)();

	len_ = swprintf_s(str_, L"%u", u);
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N, wchar_t>::StackString(const float f)
	: len_(24) // represent any float.
{
	X_ASSERT(len_ < N, "unsigned val does not fit into stackstring of size %d.", len_, N)();

	wchar_t text[64];

	size_t l = swprintf_s(text, L"%f", f);

	while (l > 0 && text[l - 1] == L'0')
		text[--l] = L'\0';
	while (l > 0 && text[l - 1] == L'.')
		text[--l] = L'\0';

	wcscpy(str_, text);
	len_ = l;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N, wchar_t>::StackString(const unsigned __int64 u)
	: len_(24)
{
	X_ASSERT(len_ < N, "unsigned __int64 does not fit into stackstring of size %d.", len_, N)();

	len_ = swprintf_s(str_, L"%I64u", u);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N, wchar_t>::StackString(const __int64 u)
	: len_(24)
{
	X_ASSERT(len_ < N, "__int64 does not fit into stackstring of size %d.", len_, N)();

	len_ = swprintf_s(str_, L"%I64d", u);
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
void StackString<N, wchar_t>::append(wchar_t ch, size_t count)
{
	X_ASSERT(len_ + count < N, "Cannot append %d character. Not enough space left.", count)(len_, N);
	
	for (size_t i = 0; i < count; i++) {
		str_[len_ + i] = ch;
	}

	len_ += count;
	str_[len_] = 0;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
void StackString<N, wchar_t>::append(const wchar_t* str)
{
	append(str, strUtil::strlen(str));
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
void StackString<N, wchar_t>::append(const wchar_t* str, size_t count)
{
	X_ASSERT(len_ + count < N, "Cannot append %d character(s) from string \"%s\". Not enough space left.", count, str)(len_, N);
	memcpy(str_ + len_, str, count * sizeof(wchar_t));
	len_ += count;
	str_[len_] = 0;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
void StackString<N, wchar_t>::append(const wchar_t* str, const wchar_t* end)
{
	append(str, safe_static_cast<size_t>(end - str));
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
void StackString<N, wchar_t>::appendFmt(const wchar_t* format, ...)
{
	va_list args;
	va_start(args, format);

	const int charactersWritten = _vsnwprintf_s(str_ + len_, N - len_, _TRUNCATE, format, args);
	if (charactersWritten < 0)
	{
		X_WARNING("StackString", "String truncation occurred during append operation.");
		len_ = N - 1;
		str_[len_] = 0;
	}
	else
	{
		len_ += charactersWritten;
	}

	va_end(args);
}

template <size_t N>
void StackString<N, wchar_t>::appendFmt(const wchar_t* format, va_list args)
{
	const int charactersWritten = _vsnwprintf_s(str_ + len_, N - len_, _TRUNCATE, format, args);
	if (charactersWritten < 0)
	{
		X_WARNING("StackString", "String truncation occurred during append operation.");
		len_ = N - 1;
		str_[len_] = 0;
	}
	else
	{
		len_ += charactersWritten;
	}
}



// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
void StackString<N, wchar_t>::set(const wchar_t* str)
{
	size_t len = core::Min<size_t>(N - 1, strUtil::strlen(str));

	X_ASSERT(len < N, "String of length %d does not fit into StackString of size %d.", len, N)();

	memcpy(str_, str, len * sizeof(wchar_t));
	len_ = len;
	str_[len_] = 0;
}


template <size_t N>
void StackString<N, wchar_t>::set(const wchar_t* const beginInclusive, const wchar_t* const endExclusive)
{
	size_t len = core::Min<size_t>(N - 1, (endExclusive - beginInclusive));

	X_ASSERT(len < N, "String of length %d does not fit into StackString of size %d.", len, N)();

	memcpy(str_, beginInclusive, len * sizeof(wchar_t));
	len_ = len;
	str_[len_] = 0;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

template <size_t N>
bool StackString<N, wchar_t>::replace(const wchar_t* start, const wchar_t* original, const wchar_t* replacement)
{
	X_ASSERT(wcscmp(original, replacement) != 0, "Replace operation cannot be performed. Strings are identical.")(original, replacement);
	X_ASSERT(start >= begin() && start <= end(), "start dose not point to a section of the string")(start, original, replacement);

	// find the position of the string to replace
	const size_t originalLength = strUtil::strlen(original);
	const wchar_t* pos = strUtil::Find(start, str_ + len_, original, originalLength);
	if (!pos)
		return false;

	wchar_t* const replacePos = const_cast<wchar_t*>(pos);

	// adjust the length of our string, assuming that the string has been replaced
	const size_t replacementLength = strUtil::strlen(replacement);
	const size_t newLength = len_ + replacementLength - originalLength;
	X_ASSERT(newLength < N, "Cannot replace \"%s\" with \"%s\" in string \"%s\". Not enough space left.", original, replacement, str_)(len_, newLength, N);

	// move characters so that the replacement fits in-between
	const size_t toCopy = safe_static_cast<size_t>((str_ + len_) - (replacePos + originalLength));
	memmove(replacePos + replacementLength, replacePos + originalLength, toCopy * sizeof(wchar_t));

	// replace characters
	memcpy(replacePos, replacement, replacementLength * sizeof(wchar_t));

	str_[newLength] = 0;
	len_ = newLength;

	return true;
}

template <size_t N>
bool StackString<N, wchar_t>::replace(const wchar_t* original, const wchar_t* replacement)
{
	// find the position of the string to replace
	const size_t originalLength = strUtil::strlen(original);

	if (originalLength == 0)
		return true;

	X_ASSERT(wcscmp(original, replacement) != 0, "Replace operation cannot be performed. Strings are identical.")(original, replacement);

	const wchar_t* pos = strUtil::Find(str_, str_ + len_, original, originalLength);
	if (!pos)
		return false;

	wchar_t* const replacePos = const_cast<wchar_t*>(pos);

	// adjust the length of our string, assuming that the string has been replaced
	const size_t replacementLength = strUtil::strlen(replacement);
	const size_t newLength = len_ + replacementLength - originalLength;
	X_ASSERT(newLength < N, "Cannot replace \"%s\" with \"%s\" in string \"%s\". Not enough space left.", original, replacement, str_)(len_, newLength, N);

	// move characters so that the replacement fits in-between
	const size_t toCopy = safe_static_cast<size_t>((str_ + len_) - (replacePos + originalLength));
	memmove(replacePos + replacementLength, replacePos + originalLength, toCopy * sizeof(wchar_t));

	// replace characters
	memcpy(replacePos, replacement, replacementLength * sizeof(wchar_t));

	str_[newLength] = 0;
	len_ = newLength;

	return true;
}



template <size_t N>
bool StackString<N, wchar_t>::replace(const wchar_t original, const wchar_t replacement)
{
	// find me baby
	wchar_t* start = str_;
	while (start < end()) {
		if (*start == original) {
			*start = replacement;
			return true;
		}
		++start;
	}
	return false;
}

template <size_t N>
size_t StackString<N, wchar_t>::replaceAll(const wchar_t* original, const wchar_t* replacement)
{
	for (size_t count = 0;; ++count)
	{
		if (!replace(original, replacement))
			return count;
	}
}

/// Replaces all occurrences of a character, and returns the number of occurrences replaced.
template <size_t N>
size_t StackString<N, wchar_t>::replaceAll(const wchar_t original, const wchar_t replacement)
{
	// find me baby
	size_t count = 0;
	wchar_t* start = str_;
	while (start < end()) {
		if (*start == original) {
			*start = replacement;
			count++;
		}
		++start;
	}
	return count;
}


template <size_t N>
void StackString<N, wchar_t>::trimWhitespace(void)
{
	const wchar_t* pos = strUtil::FindNonWhitespace(str_, str_ + len_);

	// only white space.
	if (!pos)
	{
		len_ = 0;
		str_[0] = 0;
		return;
	}

	// string contains at least one non-whitespace character
	if (pos > str_)
	{
		// there are whitespaces to the left, trim them
		memmove(str_, pos, len_ * sizeof(wchar_t));
		len_ -= safe_static_cast<size_t>(pos - str_);
	}

	pos = strUtil::FindLastNonWhitespace(str_, str_ + len_);
	if (pos && pos < str_ + len_ - 1)
	{
		// there are whitespaces to the right, trim them
		len_ = safe_static_cast<size_t>(pos - str_ + 1);
		str_[len_] = 0;
	}
}


template <size_t N>
void StackString<N, wchar_t>::trimCharacter(wchar_t character)
{
	const wchar_t* pos = strUtil::FindNon(str_, str_ + len_, character);
	if (!pos)
	{
		// string contains only the given character
		len_ = 0;
		str_[0] = 0;
		return;
	}

	// string contains at least one non-given character
	if (pos > str_)
	{
		// there are given characters to the left, trim them
		memmove(str_, pos, len_ * sizeof(wchar_t));
		len_ -= safe_static_cast<size_t>(pos - str_);
	}

	pos = strUtil::FindLastNon(str_, str_ + len_, character);
	if (pos < str_ + len_ - 1)
	{
		// there are given characters to the right, trim them
		len_ = safe_static_cast<size_t>(pos - str_ + 1);
		str_[len_] = 0;
	}
}

template <size_t N>
void StackString<N, wchar_t>::stripTrailing(const wchar_t c)
{
	size_t i;

	for (i = len_; i > 0 && str_[i - 1] == c; i--) {
		str_[i - 1] = L'\0';
		len_--;
	}
}


template <size_t N>
void StackString<N, wchar_t>::trimRight(const wchar_t* pos)
{
	len_ = safe_static_cast<size_t>(pos - str_);
	str_[len_] = 0;
}


template <size_t N>
void StackString<N, wchar_t>::trimRight(wchar_t ch)
{
	const wchar_t* pos = find(ch);
	if (pos != nullptr)
		trimRight(pos);
}


template <size_t N>
StackString<N, wchar_t>& StackString<N, wchar_t>::trim(void)
{
	return trimRight().trimLeft();
}


template <size_t N>
StackString<N, wchar_t>& StackString<N, wchar_t>::trimLeft(void)
{
	// we just loop over the string while there is white space and we are inside the string.
	const wchar_t* str = str_;

	while (*str && strUtil::IsWhitespaceW(*const_cast<wchar_t*>(str))) {
		str++;
	}

	// if they not equal we found white space.
	if (str != str_)
	{
		size_t Off = (size_t)(str - str_);
		size_t NewLength = this->length() - Off;

		memmove(str_, str_ + Off, NewLength * sizeof(wchar_t));

		str_[NewLength] = L'\0';

		len_ = NewLength;
	}

	return *this;
}


// removes any leading white space chars.
template <size_t N>
StackString<N, wchar_t>& StackString<N, wchar_t>::trimRight(void)
{
	// we want to start at the end and continue untill no more whitespace or end :)
	const wchar_t* str = this->end() - 1;
	const wchar_t* start = this->begin();

	while (str > start && strUtil::IsWhitespaceW(*const_cast<wchar_t*>(str))) {
		--str;
	}

	if (str != end())
	{
		// trim me baby
		size_t NewLength = (size_t)(str - str_) + 1;
		str_[NewLength] = 0;

		len_ = NewLength;
	}

	return *this;
}


template <size_t N>
void StackString<N, wchar_t>::clear(void)
{
	len_ = 0;
	str_[0] = 0;
}


template <size_t N>
inline bool StackString<N, wchar_t>::isEqual(const wchar_t* other) const
{
	return strUtil::IsEqual(str_, str_ + len_, other, other + strUtil::strlen(other));
}


template <size_t N>
inline const wchar_t* StackString<N, wchar_t>::find(wchar_t ch) const
{
	return strUtil::Find(str_, str_ + len_, ch);
}


template <size_t N>
inline const wchar_t* StackString<N, wchar_t>::findLast(wchar_t ch) const
{
	return strUtil::FindLast(str_, str_ + len_, ch);
}


template <size_t N>
inline const wchar_t* StackString<N, wchar_t>::find(const wchar_t* string) const
{
	return strUtil::Find(str_, str_ + len_, string);
}

template <size_t N>
inline const wchar_t* StackString<N, wchar_t>::findCaseInsen(wchar_t ch) const
{
	return strUtil::FindCaseInsensitive(str_, str_ + len_, ch);
}

template <size_t N>
inline const wchar_t* StackString<N, wchar_t>::findCaseInsen(const wchar_t* string) const
{
	return strUtil::FindCaseInsensitive(str_, str_ + len_, string);
}



template <size_t N>
inline bool StackString<N, wchar_t>::operator==(const StackString& oth) const
{
	return isEqual(oth.c_str());
}

template <size_t N>
inline bool StackString<N, wchar_t>::operator!=(const StackString& oth) const
{
	return !isEqual(oth.c_str());
}

template <size_t N>
inline StackString<N, wchar_t>& StackString<N, wchar_t>::operator=(const StackString& oth)
{
	memcpy(str_, oth.str_, oth.len_ * sizeof(wchar_t));
	len_ = oth.len_;
	str_[len_] = 0;
	return *this;
}


template <size_t N>
inline wchar_t& StackString<N, wchar_t>::operator[](size_t i)
{
	// allow access to the null terminator
	X_ASSERT(i <= len_, "Character index %d cannot be accessed. Subscript out of range.", i)(N, str_, len_);
	return str_[i];
}


template <size_t N>
inline const wchar_t& StackString<N, wchar_t>::operator[](size_t i) const
{
	// allow access to the null terminator
	X_ASSERT(i <= len_, "Character index %d cannot be accessed. Subscript out of range.", i)(N, str_, len_);
	return str_[i]; 
}


template <size_t N>
inline const wchar_t* StackString<N, wchar_t>::c_str(void) const
{
	return str_;
}


template <size_t N>
inline size_t StackString<N, wchar_t>::length(void) const
{
	return len_;
}

template <size_t N>
inline bool StackString<N, wchar_t>::isEmpty(void) const
{
	return len_ == 0;
}

template <size_t N>
inline bool StackString<N, wchar_t>::isNotEmpty(void) const
{
	return len_ > 0;
}

template <size_t N>
inline void StackString<N, wchar_t>::toLower(void)
{
	for (size_t i = 0; i<len_; ++i)
	{
		str_[i] = safe_static_cast<wchar_t>(tolower(str_[i]));
	}
}

template <size_t N>
inline void StackString<N, wchar_t>::toUpper(void)
{
	for (size_t i = 0; i<len_; ++i)
	{
		str_[i] = safe_static_cast<wchar_t>(toupper(str_[i]));
	}
}

template <size_t N>
inline const wchar_t* StackString<N, wchar_t>::begin(void) const
{
	return str_;
}

template <size_t N>
inline const wchar_t* StackString<N, wchar_t>::end(void) const
{
	return str_ + this->len_;
}

template <size_t N>
inline size_t StackString<N, wchar_t>::capacity(void) const
{
	return N;
}