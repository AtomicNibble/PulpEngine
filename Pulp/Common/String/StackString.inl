
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N>::StackString(void)
	: len_(0)
{
	str_[0] = 0;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N>::StackString(const char* const str)
	: len_(strUtil::strlen(str))
{
	X_ASSERT(len_ < N, "String \"%s\" does not fit into StackString of size %d.", str, N)();
	memcpy(str_, str, len_+1);
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------


template <size_t N>
StackString<N>::StackString(const StringRange& range)
	: len_(safe_static_cast<uint32_t>(range.GetLength()))
{
	X_ASSERT(len_ < N, "StringRange does not fit into StackString of size %d.", N)();

	// ranges do not necessarily contain a null-terminator, hence we add it manually
	memcpy(str_, range.GetStart(), len_);
	str_[len_] = '\0';
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N>::StackString(const char* const beginInclusive, const char* const endExclusive)
	: len_(safe_static_cast<uint32_t>(endExclusive - beginInclusive))
{
	X_ASSERT(len_ < N, "String of length %d does not fit into StackString of size %d.", len_, N)();
	memcpy(str_, beginInclusive, len_);
	str_[len_] = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N>::StackString(const bool b)
: len_(1)
{
	X_ASSERT(len_ < N, "bool val does not fit into stackstring of size %d.", len_, N)();

	str_[0] = b ? '1' : '0';
	str_[1] = '\0';
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N>::StackString(const char c)
: len_(1)
{
	X_ASSERT(len_ < N, "char val does not fit into stackstring of size %d.", len_, N)();

	str_[0] = c;
	str_[1] = '\0';
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N>::StackString(const int i)
: len_(12)
{
	X_ASSERT(len_ < N, "int val does not fit into stackstring of size %d.", len_, N)();

	len_ = sprintf_s(str_, "%d", i);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N>::StackString(const unsigned u)
: len_(12)
{
	X_ASSERT(len_ < N, "unsigned val does not fit into stackstring of size %d.", len_, N)();

	len_ = sprintf_s(str_, "%u", u);
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N>::StackString(const float f)
: len_(24) // represent any float.
{
	X_ASSERT(len_ < N, "unsigned val does not fit into stackstring of size %d.", len_, N)();

	char text[64];

	uint32_t l = sprintf_s(text, "%f", f);

	while (l > 0 && text[l - 1] == '0')
		text[--l] = '\0';
	while (l > 0 && text[l - 1] == '.')
		text[--l] = '\0';

	strcpy(str_, text);
	len_ = l;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N>::StackString(const unsigned __int64 u)
: len_(24)
{
	X_ASSERT(len_ < N, "unsigned __int64 does not fit into stackstring of size %d.", len_, N)();

	len_ = sprintf_s(str_, "%I64u", u);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
StackString<N>::StackString(const __int64 u)
: len_(24)
{
	X_ASSERT(len_ < N, "__int64 does not fit into stackstring of size %d.", len_, N)();

	len_ = sprintf_s(str_, "%I64d", u);
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
void StackString<N>::append(char ch, unsigned int count)
{
	memset(str_ + len_, ch, count);
	len_ += count;
	str_[len_] = 0;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
void StackString<N>::append(const char* str)
{
	append(str, strUtil::strlen(str));
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
void StackString<N>::append(const char* str, unsigned int count)
{
	X_ASSERT(len_ + count < N, "Cannot append %d character(s) from string \"%s\". Not enough space left.", count, str)(len_, N);
	memcpy(str_ + len_, str, count);
	len_ += count;
	str_[len_] = 0;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
void StackString<N>::append(const char* str, const char* end)
{
	append(str, safe_static_cast<unsigned int>(end - str));
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <size_t N>
void StackString<N>::appendFmt(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	const int charactersWritten = _vsnprintf_s(str_ + len_, N - len_, _TRUNCATE, format, args);
	if (charactersWritten < 0)
	{
		X_WARNING("StackString", "String truncation occurred during append operation.");
		len_ = N-1;
		str_[len_] = 0;
	}
	else
	{
		len_ += charactersWritten;
	}
}

template <size_t N>
void StackString<N>::appendFmt(const char* format, va_list args)
{
	const int charactersWritten = _vsnprintf_s(str_ + len_, N - len_, _TRUNCATE, format, args);
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
void StackString<N>::set(const char* str)
{
	uint32_t len = core::Min<uint32_t>(N - 1, strUtil::strlen(str));
	
	X_ASSERT(len < N, "String of length %d does not fit into StackString of size %d.", len, N)();

	memcpy(str_, str, len);
	len_ = len;
	str_[len_] = 0;
}


template <size_t N>
void StackString<N>::set(const char* const beginInclusive, const char* const endExclusive)
{
	uint32_t len = core::Min<uint32_t>(N - 1, 
		safe_static_cast<uint32_t, size_t>(endExclusive - beginInclusive));

	X_ASSERT(len < N, "String of length %d does not fit into StackString of size %d.", len, N)();

	memcpy(str_, beginInclusive, len);
	len_ = len;
	str_[len_] = 0;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

template <size_t N>
bool StackString<N>::replace(const char* start, const char* original, const char* replacement)
{
	X_ASSERT(strcmp(original, replacement) != 0, "Replace operation cannot be performed. Strings are identical.")(original, replacement);
	X_ASSERT(start >= begin() && start <= end(), "start dose not point to a section of the string")(start, original, replacement);

	// find the position of the string to replace
	const uint32_t originalLength = strUtil::strlen(original);
	const char* pos = strUtil::Find(start, str_ + len_, original, originalLength);
	if (!pos)
		return false;

	char* const replacePos = const_cast<char*>(pos);

	// adjust the length of our string, assuming that the string has been replaced
	const uint32_t replacementLength = strUtil::strlen(replacement);
	const uint32_t newLength = len_ + replacementLength - originalLength;
	X_ASSERT(newLength < N, "Cannot replace \"%s\" with \"%s\" in string \"%s\". Not enough space left.", original, replacement, str_)(len_, newLength, N);

	// move characters so that the replacement fits in-between
	const uint32_t toCopy = safe_static_cast<uint32_t>((str_ + len_) - (replacePos + originalLength));
	memmove(replacePos + replacementLength, replacePos + originalLength, toCopy);

	// replace characters
	memcpy(replacePos, replacement, replacementLength);

	str_[newLength] = 0;
	len_ = newLength;

	return true;
}

template <size_t N>
bool StackString<N>::replace(const char* original, const char* replacement)
{
	X_ASSERT(strcmp(original, replacement) != 0, "Replace operation cannot be performed. Strings are identical.")(original, replacement);

	// find the position of the string to replace
	const uint32_t originalLength = strUtil::strlen(original);
	const char* pos = strUtil::Find(str_, str_ + len_, original, originalLength);
	if (!pos)
		return false;

	char* const replacePos = const_cast<char*>(pos);

	// adjust the length of our string, assuming that the string has been replaced
	const uint32_t replacementLength = strUtil::strlen(replacement);
	const uint32_t newLength = len_ + replacementLength - originalLength;
	X_ASSERT(newLength < N, "Cannot replace \"%s\" with \"%s\" in string \"%s\". Not enough space left.", original, replacement, str_)(len_, newLength, N);

	// move characters so that the replacement fits in-between
	const uint32_t toCopy = safe_static_cast<uint32_t>((str_ + len_) - (replacePos + originalLength));
	memmove(replacePos + replacementLength, replacePos + originalLength, toCopy);

	// replace characters
	memcpy(replacePos, replacement, replacementLength);

	str_[newLength] = 0;
	len_ = newLength;

	return true;
}



template <size_t N>
bool StackString<N>::replace(const char original, const char replacement)
{
	// find me baby
	char* start = str_;
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
unsigned int StackString<N>::replaceAll(const char* original, const char* replacement)
{
	for (unsigned int count=0; ; ++count)
	{
		if (!replace(original, replacement))
			return count;
	}
}

/// Replaces all occurrences of a character, and returns the number of occurrences replaced.
template <size_t N>
unsigned int StackString<N>::replaceAll(const char original, const char replacement)
{
	// find me baby
	unsigned int count = 0;
	char* start = str_;
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
void StackString<N>::trimWhitespace(void)
{
	const char* pos = strUtil::FindNonWhitespace(str_, str_ + len_);

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
		memmove(str_, pos, len_);
		len_ -= safe_static_cast<unsigned int>(pos - str_);
	}

	pos = strUtil::FindLastNonWhitespace(str_, str_ + len_);
	if (pos && pos < str_+len_-1)
	{
		// there are whitespaces to the right, trim them
		len_ = safe_static_cast<unsigned int>(pos - str_ + 1);
		str_[len_] = 0;
	}
}


template <size_t N>
void StackString<N>::trimCharacter(char character)
{
	const char* pos = strUtil::FindNon(str_, str_ + len_, character);
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
		memmove(str_, pos, len_);
		len_ -= safe_static_cast<unsigned int>(pos - str_);
	}

	pos = strUtil::FindLastNon(str_, str_ + len_, character);
	if (pos < str_+len_-1)
	{
		// there are given characters to the right, trim them
		len_ = safe_static_cast<unsigned int>(pos - str_ + 1);
		str_[len_] = 0;
	}
}

template <size_t N>
void StackString<N>::stripTrailing( const char c ) 
{
	uint32_t i;

	for( i = len_; i > 0 && str_[ i - 1 ] == c; i-- ) {
		str_[ i - 1 ] = '\0';
		len_--;
	}
}


template <size_t N>
void StackString<N>::trimRight(const char* pos)
{
	len_ = safe_static_cast<uint32_t>(pos - str_);
	str_[len_] = 0;
}


template <size_t N>
void StackString<N>::trimRight(char ch)
{
	const char* pos = Find(ch);
	if (pos != nullptr)
		TrimRight(pos);
}


template <size_t N>
StackString<N>& StackString<N>::trim(void)
{
	return trimRight().trimLeft();
}


template <size_t N>
StackString<N>& StackString<N>::trimLeft(void)
{
	// we just loop over the string while there is white space and we are inside the string.
	const char* str = str_;

	while (*str && strUtil::IsWhitespace((char)*str))
		str++;

	// if they not equal we found white space.
	if (str != str_)
	{
		size_t Off = (size_t)(str - str_);
		size_t NewLength = this->length() - Off;

		memmove(str_, str_ + Off, NewLength);

		str_[NewLength] = '\0';

		len_ = safe_static_cast<uint32_t, size_t>(NewLength);
	}

	return *this;
}


// removes any leading white space chars.
template <size_t N>
StackString<N>& StackString<N>::trimRight(void)
{
	// we want to start at the end and continue untill no more whitespace or end :)
	const char* str = this->end() - 1;
	const char* start = this->begin();

	while (str > start && strUtil::IsWhitespace((char)*str))
		--str;

	if (str != end())
	{
		// trim me baby
		size_t NewLength = (size_t)(str - str_) + 1;
		str_[NewLength] = 0;

		len_ = safe_static_cast<uint32_t,size_t>(NewLength);
	}

	return *this;
}


template <size_t N>
void StackString<N>::clear(void)
{
	len_ = 0;
	str_[0] = 0;
}


template <size_t N>
inline bool StackString<N>::isEqual(const char* other) const
{
	return strUtil::IsEqual(str_, str_ + len_, other, other + strlen(other));
}


template <size_t N>
inline const char* StackString<N>::find(char ch) const
{
	return strUtil::Find(str_, str_ + len_, ch);
}


template <size_t N>
inline const char* StackString<N>::findLast(char ch) const
{
	return strUtil::FindLast(str_, str_ + len_, ch);
}


template <size_t N>
inline const char* StackString<N>::find(const char* string) const
{
	return strUtil::Find(str_, str_ + len_, string);
}

template <size_t N>
inline const char* StackString<N>::findCaseInsen(char ch) const
{
	return strUtil::FindCaseInsensitive(str_, str_ + len_, ch);
}

template <size_t N>
inline const char* StackString<N>::findCaseInsen(const char* string) const
{
	return strUtil::FindCaseInsensitive(str_, str_ + len_, string);
}



template <size_t N>
inline bool StackString<N>::operator==(const StackString& oth) const
{
	return isEqual(oth.c_str());
}

template <size_t N>
inline bool StackString<N>::operator!=(const StackString& oth) const
{
	return !isEqual(oth.c_str());
}

template <size_t N>
inline StackString<N>& StackString<N>::operator=(const StackString& oth)
{
	memcpy(str_, oth.str_, oth.len_);
	len_ = oth.len_;
	str_[len_] = 0;
	return *this;
}


template <size_t N>
inline char& StackString<N>::operator[](uint32_t i)
{
	// allow access to the null terminator
	X_ASSERT(i <= len_, "Character %d cannot be accessed. Subscript out of range.", i)(N, str_, len_);
	return str_[i];
}


template <size_t N>
inline const char& StackString<N>::operator[](uint32_t i) const
{
	// allow access to the null terminator
	X_ASSERT(i <= len_, "Character %d cannot be accessed. Subscript out of range.", i)(N, str_, len_);
	return str_[i];.
}


template <size_t N>
inline const char* StackString<N>::c_str(void) const
{
	return str_;
}


template <size_t N>
inline uint32_t StackString<N>::length(void) const
{
	return len_;
}

template <size_t N>
inline bool StackString<N>::isEmpty(void) const
{
	return len_ == 0;
}

template <size_t N>
inline bool StackString<N>::isNotEmpty(void) const
{
	return len_ > 0;
}

template <size_t N>
inline void StackString<N>::toLower(void)
{
	for (unsigned int i=0; i<len_; ++i)
	{
		str_[i] = safe_static_cast<char>(tolower(str_[i]));
	}
}

template <size_t N>
inline void StackString<N>::toUpper(void)
{
	for (unsigned int i=0; i<len_; ++i)
	{
		str_[i] = safe_static_cast<char>(toupper(str_[i]));
	}
}

template <size_t N>
inline const char* StackString<N>::begin(void) const
{
	return str_;
}

template <size_t N>
inline const char* StackString<N>::end(void) const
{
	return str_ + this->len_;
}

template <size_t N>
inline uint32_t StackString<N>::capacity(void) const
{
	return N;
}