







template<typename CharT>
StringRef<CharT>::StringRef()
{
	SetEmpty();
}

// from another string object
template<typename CharT>
StringRef<CharT>::StringRef(const StrT& str)
{
	X_ASSERT(str.header()->refCount != 0, "can't constuct a string from one that has a ref count of 0")();

	if (str.header()->refCount >= 0)
	{
		str_ = str.str_;
		header()->addRef();
	}
	else
	{
		SetEmpty(); 
	}
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
	X_ASSERT(numRepeat > 0, "string constructed with a char repeat value of 0.")(ch,numRepeat);
	
	if (numRepeat > 0)
	{
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
	if (len > 0)
	{
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
	if (len > 0)
	{
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
	if (len > 0)
	{
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
	return header()->length;
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
	return header()->allocSize;
}

template<typename CharT>
bool StringRef<CharT>::isEmpty(void) const
{
	return length() == 0;
}

// clears the string and de-inc the ref
template<typename CharT>
void StringRef<CharT>::clear(void)
{
	if (length() == 0)
		return;
	if (header()->refCount >= 0) // check for the -1 ref.
		free();

	X_ASSERT(length() == 0, "Failed to clear string")(length());
	X_ASSERT(header()->refCount < 0 || capacity() == 0, "string is not pointing to empty data")();
}

// Sets the capacity of the string to a number at least as great as a specified number.
template<typename CharT>
void StringRef<CharT>::reserve(size_type size)
{
	if (size > capacity())
	{
		XStrHeader* pOldData =header();
		Allocate(size);
		_copy(str_, pOldData->GetChars(), pOldData->length);
		header()->length = pOldData->length;
		str_[pOldData->length] = 0;
		freeData(pOldData);
	}
}

template <class T>
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
	if (length() != capacity())
	{
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
		if (thisRef < 0)
		{
			if (srcRef < 0)
			{
				// both are null :|
				// what do you want from me!
			}
			else
			{
				str_ = str.str_;
				header()->addRef();
			}
		}
		else if (srcRef < 0)
		{
			free();
			str_ = str.str_;
		}
		else
		{
			// free the current string, and set + ref.
			free();
			str_ = str.str_;
			header()->addRef();
		}
	}
	return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::operator=(value_type ch)
{
	// single char for my booty.
	_Assign(&ch,1);
	return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::operator=(const_str str)
{
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
	ConcatenateInPlace(&ch,1);
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
	for (i = 0; i<len; ++i) {
		str_[i] = safe_static_cast<CharT>(tolower(str_[i]));
	}
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::toUpper(void)
{
	makeUnique();
	size_type i, len;

	len = length();
	for ( i = 0; i<len; ++i) {
		str_[i] = safe_static_cast<CharT>(toupper(str_[i]));
	}
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

	const value_type temp[2] = { ch, 0 };
	return trimRight(temp).trimLeft(temp);
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trim(const_str sCharSet)
{
	makeUnique();

	return trimRight(sCharSet).trimLeft(sCharSet);
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trimLeft(void)
{
	const value_type *str = str_;
	while ((*str != 0) && strUtil::IsWhitespace((unsigned char)*str))
		str++;

	if (str != str_)
	{
		size_type nOff = (size_type)(str - str_); // m_str can change in _MakeUnique
		makeUnique();
		size_type nNewLength = length() - nOff;
		_move(str_, str_ + nOff, nNewLength + 1);
		header()->length = safe_static_cast<int, size_t>(nNewLength);
		str_[nNewLength] = 0;
	}

	return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trimLeft(value_type ch)
{
	const value_type temp[2] = { ch, 0 };
	return trimLeft(temp);
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trimLeft(const_str sCharSet)
{
	if (!sCharSet || !(*sCharSet))
		return *this;

	const value_type *str = str_;
	while ((*str != 0) && (_strchr(sCharSet, *str) != 0))
		str++;

	if (str != str_)
	{
		size_type nOff = (size_type)(str - str_); // str_ can change in _MakeUnique
		makeUnique();
		size_type nNewLength = length() - nOff;
		_move(str_, str_ + nOff, nNewLength + 1);
		header()->length = nNewLength;
		str_[nNewLength] = 0;
	}

	return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trimRight(void)
{
	if (length() < 1)
		return *this;

	const value_type *last = str_ + length() - 1;
	const value_type *str = last;
	while ((str != str_) && strUtil::IsWhitespace((unsigned char)*str))
		str--;

	if (str != last)		// something changed?
	{
		// Just shrink length of the string.
		size_type nNewLength = (size_type)(str - str_) + 1; // str_ can change in _MakeUnique
		makeUnique();
		header()->length = safe_static_cast<int, size_t>(nNewLength);
		str_[nNewLength] = 0;
	}

	return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trimRight(value_type ch)
{
	const value_type temp[2] = { ch, 0 };
	return trimRight(temp);
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::trimRight(const_str sCharSet)
{
	if (!sCharSet || !(*sCharSet) || length() < 1)
		return *this;

	const value_type *last = str_ + length() - 1;
	const value_type *str = last;
	while ((str != str_) && (_strchr(sCharSet, *str) != 0))
		str--;

	if (str != last)
	{
		// Just shrink length of the string.
		size_type nNewLength = (size_type)(str - str_) + 1; // str_ can change in _MakeUnique
		makeUnique();
		header()->length = nNewLength;
		str_[nNewLength] = 0;
	}

	return *this;
}


// =============================================================================

template <class CharT>
inline StringRef<CharT>& StringRef<CharT>::append(const_str _Ptr)
{
	*this += _Ptr;
	return *this;
}

template <class CharT>
inline StringRef<CharT>& StringRef<CharT>::append(const_str _Ptr, size_type nCount)
{
	_ConcatenateInPlace(_Ptr, nCount);
	return *this;
}

template <class CharT>
inline StringRef<CharT>& StringRef<CharT>::append(const StringRef<CharT>& _Str,
	size_type off, size_type nCount)
{
	size_type len = _Str.length();
	if (off > len)
		return *this;
	if (off + nCount > len)
		nCount = len - off;
	_ConcatenateInPlace(_Str.str_ + off, nCount);
	return *this;
}

template <class CharT>
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
	CheckConvert(D& d)
	: dest(&d) {}

	template<class S>
	D& operator=(S const& s)
	{
		return check_convert(*dest, s);
	}

protected:
	D*	dest;
};

template<class D>
inline CheckConvert<D> check_convert(D& d)
{
	return d;
}

template <class CharT>
inline StringRef<CharT>& StringRef<CharT>::append(size_type nCount, value_type _Ch)
{
	if (nCount > 0)
	{
		if (length() + nCount >= capacity())
		{
			XStrHeader* pOldData = header();
			Allocate(length() + nCount);
			_copy(str_, pOldData->getChars(), pOldData->length);
			_set(str_ + pOldData->length, _Ch, nCount);
			freeData(pOldData);
		}
		else
		{
			size_type nOldLength = length();
			_set(str_ + nOldLength, _Ch, nCount);
			check_convert(header()->length) = nOldLength + nCount;
			str_[length()] = 0; // Make null terminated string.
		}
	}
	return *this;
}

template <class CharT>
inline StringRef<CharT>& StringRef<CharT>::append(const_iterator _First, const_iterator _Last)
{
	append(_First, (size_type)(_Last - _First));
	return *this;
}


// =============================================================================

template <class CharT>
inline StringRef<CharT>& StringRef<CharT>::assign(const_str _Ptr)
{
	*this = _Ptr;
	return *this;
}

template <class CharT>
inline StringRef<CharT>& StringRef<CharT>::assign(const_str _Ptr, size_type nCount)
{
	size_type len = strnlen(_Ptr, nCount);
	_Assign(_Ptr, len);
	return *this;
}

template <class CharT>
inline StringRef<CharT>& StringRef<CharT>::assign(const StringRef<CharT>& _Str,
	size_type off, size_type nCount)
{
	size_type len = _Str.length();
	if (off > len)
		return *this;
	if (off + nCount > len)
		nCount = len - off;
	_Assign(_Str.str_ + off, nCount);
	return *this;
}

template <class CharT>
inline StringRef<CharT>& StringRef<CharT>::assign(const StringRef<CharT>& _Str)
{
	*this = _Str;
	return *this;
}

template <class CharT>
inline StringRef<CharT>& StringRef<CharT>::assign(size_type nCount, value_type _Ch)
{
	if (nCount >= 1)
	{
		Allocate(nCount);
		_set(str_, _Ch, nCount);
	}
	return *this;
}

template <class CharT>
inline StringRef<CharT>& StringRef<CharT>::assign(const_iterator _First, const_iterator _Last)
{
	assign(_First, (size_type)(_Last - _First));
	return *this;
}

// =============================================================================

template <class T>
inline StringRef<T>& StringRef<T>::replace(size_type pos, size_type count, const_str strNew)
{
	return replace(pos, count, strNew, strlen(strNew));
}

template <class T>
inline StringRef<T>& StringRef<T>::replace(size_type pos, size_type count, const_str strNew, size_type count2)
{
	erase(pos, count);
	insert(pos, strNew, count2);
	return *this;
}

template <class T>
inline StringRef<T>& StringRef<T>::replace(size_type pos, size_type count, size_type nNumChars, value_type chNew)
{
	erase(pos, count);
	insert(pos, nNumChars, chNew);
	return *this;
}

template <class T>
inline StringRef<T>& StringRef<T>::replace(value_type chOld, value_type chNew)
{
	if (chOld != chNew)
	{
		makeUnique();
		value_type* strend = str_ + length();
		for (value_type* str = str_; str != strend; ++str)
		{
			if (*str == chOld)
			{
				*str = chNew;
			}
		}
	}
	return *this;
}

template <class T>
inline StringRef<T>& StringRef<T>::replace(const_str strOld, const_str strNew)
{
	size_type nSourceLen = strlen(strOld);
	if (nSourceLen == 0)
		return *this;
	size_type nReplacementLen = strlen(strNew);

	size_type nCount = 0;
	value_type* strStart = str_;
	value_type* strEnd = str_ + length();
	value_type* strTarget;
	while (strStart < strEnd)
	{
		while ((strTarget = _strstr(strStart, strOld)) != NULL)
		{
			nCount++;
			strStart = strTarget + nSourceLen;
		}
		strStart += _strlen(strStart) + 1;
	}

	if (nCount > 0)
	{
		makeUnique();

		size_type nOldLength = length();
		size_type nNewLength = nOldLength + (nReplacementLen - nSourceLen)*nCount;
		if (capacity() < nNewLength || header()->refCount > 1)
		{
			XStrHeader* pOldData = header();
			const_str pstr = str_;
			Allocate(nNewLength);
			_copy(str_, pstr, pOldData->length);
			freeData(pOldData);
		}
		strStart = str_;
		strEnd = str_ + length();

		while (strStart < strEnd)
		{
			while ((strTarget = _strstr(strStart, strOld)) != NULL)
			{
				size_type nBalance = nOldLength - ((size_type)(strTarget - str_) + nSourceLen);
				_move(strTarget + nReplacementLen, strTarget + nSourceLen, nBalance);
				_copy(strTarget, strNew, nReplacementLen);
				strStart = strTarget + nReplacementLen;
				strStart[nBalance] = 0;
				nOldLength += (nReplacementLen - nSourceLen);
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
typename StringRef<CharT>::StrT& StringRef<CharT>::insert(size_type nIndex, size_type nCount, value_type ch)
{
	makeUnique();

	if (nIndex < 0)
		nIndex = 0;

	size_type nNewLength = length();
	if (nIndex > nNewLength)
		nIndex = nNewLength;
	nNewLength += nCount;

	if (capacity() < nNewLength)
	{
		XStrHeader* pOldData = header();
		const_str pstr = str_;
		Allocate(nNewLength);
		_copy(str_, pstr, pOldData->length + 1);
		freeData(pOldData);
	}

	_move(str_ + nIndex + nCount, str_ + nIndex, (nNewLength - nIndex));
	_set(str_ + nIndex, ch, nCount);
	header()->length = safe_static_cast<int,size_type>(nNewLength);

	return *this;
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::insert(size_type nIndex, const_str pstr)
{
	return insert(nIndex, pstr, strlen(pstr));
}

template<typename CharT>
typename StringRef<CharT>::StrT& StringRef<CharT>::insert(size_type nIndex, const_str pstr, size_type nCount)
{
	if (nIndex < 0)
		nIndex = 0;

	size_type nInsertLength = nCount;
	size_type nNewLength = length();
	if (nInsertLength > 0)
	{
		makeUnique();
		if (nIndex > nNewLength)
			nIndex = nNewLength;
		nNewLength += nInsertLength;

		if (capacity() < nNewLength)
		{
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
	if (nIndex < 0)
		nIndex = 0;
	if (count < 0 || count > length() - nIndex)
		count = length() - nIndex;
	if (count > 0 && nIndex < length())
	{
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
	return strcmp(str_, ptr);
}

// =============================================================================

template<typename CharT>
bool StringRef<CharT>::compareNoCase(const StrT& Str) const
{
	return core::strUtil::IsEqual(begin(), end(), Str.begin(), Str.end());
}


template<typename CharT>
bool StringRef<CharT>::compareNoCase(const_str ptr) const
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

template <class CharT>
inline StringRef<CharT> StringRef<CharT>::right(size_type count) const
{
//	if (count == npos)
//		return StringRef<CharT>();
//	else
	if (count > length())
		return *this;

	return StringRef<CharT>(str_ + length() - count, count);
}

template <class CharT>
inline StringRef<CharT> StringRef<CharT>::left(size_type count) const
{
//	if (count == npos)
//		return StringRef<CharT>();
//	else 
	if (count > length())
		count = length();

	return StringRef<CharT>(str_, count);
}


// =============================================================================

template <class CharT>
inline void StringRef<CharT>::_copy(value_type* dest, const value_type* src, size_type count)
{
	if (dest != src) {
		memcpy(dest, src, count*sizeof(value_type));
	}
}

template <class CharT>
inline void StringRef<CharT>::_move(value_type* dest, const value_type* src, size_type count)
{
	memmove(dest, src, count*sizeof(value_type));
}

template <class CharT>
inline void StringRef<CharT>::_set(value_type* dest, value_type ch, size_type count)
{
	memset(dest, ch, count*sizeof(value_type));
}

template <>
inline void StringRef<wchar_t>::_set(value_type* dest, value_type ch, size_type count)
{
	wmemset(dest, ch, count);
}

// =============================================================================

template <class CharT>
typename StringRef<CharT>::XStrHeader* StringRef<CharT>::header(void) const
{
	X_ASSERT_NOT_NULL(str_);

	return ((XStrHeader*)str_) - 1;
}

// dose not check current length
template <class CharT>
void StringRef<CharT>::Allocate(size_type length)
{
	X_ASSERT(length >= 0 && length <= (INT_MAX - 1), "length is invalid")(length);

	if (length == 0)
		SetEmpty();
	else
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pStrArena);

		size_type allocLen = sizeof(XStrHeader)+((length + 1)*sizeof(value_type));
		XStrHeader* pData = (XStrHeader*)X_NEW_ARRAY_OFFSET(BYTE, allocLen, gEnv->pStrArena, "StringBuf", sizeof(XStrHeader));
	//	XStrHeader* pData = (XStrHeader*)X_NEW_ARRAY(BYTE, allocLen, gEnv->pStrArena, "StringBuf");


		pData->refCount = 1;
		str_ = pData->getChars();
		pData->length = safe_static_cast<int, size_type>(length);
		pData->allocSize = safe_static_cast<int, size_type>(length);
		str_[length] = 0; // null terminated string.
	}
}



template <class CharT>
void StringRef<CharT>::free(void)
{
	if (header()->refCount >= 0) // Not empty string.
	{
		freeData(header());
		SetEmpty();
	}
}

template <class CharT>
void StringRef<CharT>::SetEmpty(void)
{
	str_ = emptyHeader()->getChars();
}

template <class CharT>
void StringRef<CharT>::makeUnique(void)
{
	if (header()->refCount > 1)
	{
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

template <class CharT>
void StringRef<CharT>::Concatenate(const_str sStr1, size_type nLen1, const_str sStr2, size_type nLen2)
{
	size_type nLen = nLen1 + nLen2;

	if (nLen1 * 2 > nLen)
		nLen = nLen1 * 2;

	if (nLen != 0)
	{
		if (nLen < 8)
			nLen = 8;

		Allocate(nLen);
		_copy(str_, sStr1, nLen1);
		_copy(str_ + nLen1, sStr2, nLen2);
		check_convert(header()->length) = nLen1 + nLen2;
		str_[nLen1 + nLen2] = 0;
	}
}

template <class CharT>
void StringRef<CharT>::ConcatenateInPlace(const_str sStr, size_type nLen)
{
	if (nLen != 0)
	{
		// Check if this string is shared (reference count greater then 1)
		// or not enough capacity to store new string.
		// Then allocate new string buffer.
		if (header()->refCount > 1 || length() + nLen > capacity())
		{
			XStrHeader* pOldData = header();
			Concatenate(str_, length(), sStr, nLen);
			freeData(pOldData);
		}
		else
		{
			_copy(str_ + length(), sStr, nLen);
			check_convert(header()->length) = header()->length + nLen;
			str_[header()->length] = 0; // Make null terminated string.
		}
	}
}


template <class CharT>
void StringRef<CharT>::_Assign(const_str sStr, size_type Len)
{
	// Check if this string is shared (reference count greater then 1) 
	// or not enough capacity to store new string.
	// Then allocate new string buffer.
	if (header()->refCount > 1 || Len > capacity())
	{
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

template <class CharT>
void StringRef<CharT>::freeData(XStrHeader* pData)
{
	if (pData->refCount >= 0) // Not empty string.
	{
		X_ASSERT(pData->refCount != 0, "invalid ref count")(pData->refCount);
		if (pData->release() <= 0)
		{
			X_ASSERT_NOT_NULL(gEnv);
			X_ASSERT_NOT_NULL(gEnv->pStrArena);

			X_DELETE(pData, gEnv->pStrArena);
		//	delete[] pData;
		}
	}
}

template <class CharT>
size_t StringRef<CharT>::strlen(const_str pStr)
{
	return core::strUtil::strlen(pStr);
}

// =============================================================================


template <class T>
inline bool operator==(const StringRef<T>& s1, const StringRef<T>& s2)
{
	return s1.compare(s2);
}

template <class T> 
inline bool operator==(const StringRef<T>& s1, const typename StringRef<T>::value_type* s2)
{
	return s1.compare(s2);
}

template <class T> 
inline bool operator==(const typename StringRef<T>::value_type* s1, const StringRef<T>& s2)
{
	return s2.compare(s1);
}

template <class T> 
inline bool operator!=(const StringRef<T>& s1, const StringRef<T>& s2)
{
	return !s1.compare(s2);
}

template <class T> 
inline bool operator!=(const StringRef<T>& s1, const typename StringRef<T>::value_type* s2)
{
	return !s1.compare(s2);
}

template <class T> 
inline bool operator!=(const typename StringRef<T>::value_type* s1, const StringRef<T>& s2)
{
	return !s2.compare(s1);
}


//! compare helpers

template <class T> 
inline bool operator<(const StringRef<T>& s1, const StringRef<T>& s2)
{
	return s1.compareInt(s2) < 0;
}

template <class T> 
inline bool operator<(const StringRef<T>& s1, const typename StringRef<T>::value_type* s2)
{
	return s1.compareInt(s2) < 0;
}

template <class T> 
inline bool operator<(const typename StringRef<T>::value_type* s1, const StringRef<T>& s2)
{
	return s2.compareInt(s1) > 0;
}

template <class T>
inline bool operator>(const StringRef<T>& s1, const StringRef<T>& s2)
{
	return s1.compareInt(s2) > 0;
}

template <class T> 
inline bool operator>(const StringRef<T>& s1, const typename StringRef<T>::value_type* s2)
{
	return s1.compareInt(s2) > 0;
}

template <class T> 
inline bool operator>(const typename StringRef<T>::value_type* s1, const StringRef<T>& s2)
{
	return s2.compareInt(s1) < 0;
}

template <class T> 
inline bool operator<=(const StringRef<T>& s1, const StringRef<T>& s2)
{
	return s1.compareInt(s2) <= 0;
}

template <class T> 
inline bool operator<=(const StringRef<T>& s1, const typename StringRef<T>::value_type* s2)
{
	return s1.compareInt(s2) <= 0;
}

template <class T> 
inline bool operator<=(const typename StringRef<T>::value_type* s1, const StringRef<T>& s2)
{
	return s2.compareInt(s1) >= 0;
}

template <class T> 
inline bool operator>=(const StringRef<T>& s1, const StringRef<T>& s2)
{
	return s1.compareInt(s2) >= 0;
}

template <class T> 
inline bool operator>=(const StringRef<T>& s1, const typename StringRef<T>::value_type* s2)
{
	return s1.compareInt(s2) >= 0;
}

template <class T> 
inline bool operator>=(const typename StringRef<T>::value_type* s1, const StringRef<T>& s2)
{
	return s2.compareInt(s1) <= 0;
} 

// =============================================================================


template <class T>
inline StringRef<T> operator+(const StringRef<T>& string1, typename StringRef<T>::value_type ch)
{
	StringRef<T> s(string1);
	s.append(1, ch);
	return s;
}

template <class T>
inline StringRef<T> operator+(typename StringRef<T>::value_type ch, const StringRef<T>& str)
{
	StringRef<T> s;
	s.reserve(str.size() + 1);
	s.append(1, ch);
	s.append(str);
	return s;
}

template <class T>
inline StringRef<T> operator+(const StringRef<T>& string1, const StringRef<T>& string2)
{
	StringRef<T> s(string1);
	s.append(string2);
	return s;
}

template <class T>
inline StringRef<T> operator+(const StringRef<T>& str1, const typename StringRef<T>::value_type* str2)
{
	StringRef<T> s(str1);
	s.append(str2);
	return s;
}

template <class T>
inline StringRef<T> operator+(const typename StringRef<T>::value_type* str1, const StringRef<T>& str2)
{
	X_ASSERT_NOT_NULL(str);

	StringRef<T> s;
	s.reserve(StringRef<T>::strlen(str1) + str2.size());
	s.append(str1);
	s.append(str2);
	return s;
}




/*
//////////////////////////////////////////////////////////////////////////
template <class T>
class StrRef
{
public:
	//////////////////////////////////////////////////////////////////////////
	// Types compatible with STL string.
	//////////////////////////////////////////////////////////////////////////
	typedef StrRef<T> _Self;
	typedef size_t size_type;
	typedef T value_type;
	typedef const value_type* const_str;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef pointer iterator;
	typedef const_pointer const_iterator;

	enum _npos_type {
		npos = (size_type)~0
	};

	//////////////////////////////////////////////////////////////////////////
	// Constructors
	//////////////////////////////////////////////////////////////////////////
	StrRef();

protected:
	StrRef(const ConstCharWrapper& str);	//ctor for strings without memory allocations
	friend class ConstCharWrapper;
public:

	StrRef(const _Self& str);
	StrRef(const _Self& str, size_type nOff, size_type nCount);
	explicit StrRef(value_type ch, size_type nRepeat = 1);
	StrRef(const_str str);
	StrRef(const_str str, size_type nLength);
	StrRef(const_iterator _First, const_iterator _Last);
	~StrRef();

	//////////////////////////////////////////////////////////////////////////
	// STL string like interface.
	//////////////////////////////////////////////////////////////////////////
	//! Operators.
	size_type length() const;
	size_type size() const;
	bool empty() const;
	void clear();  // free up the data

	//! Returns the storage currently allocated to hold the string, a value at least as large as length().
	size_type capacity() const;

	// Sets the capacity of the string to a number at least as great as a specified number.
	// nCount = 0 is shrinking string to fit number of characters in it.
	void reserve(size_type nCount = 0);

	_Self& append(const value_type* _Ptr);
	_Self& append(const value_type* _Ptr, size_type nCount);
	_Self& append(const _Self& _Str, size_type nOff, size_type nCount);
	_Self& append(const _Self& _Str);
	_Self& append(size_type nCount, value_type _Ch);
	_Self& append(const_iterator _First, const_iterator _Last);

	_Self& assign(const_str _Ptr);
	_Self& assign(const_str _Ptr, size_type nCount);
	_Self& assign(const _Self& _Str, size_type off, size_type nCount);
	_Self& assign(const _Self& _Str);
	_Self& assign(size_type nCount, value_type _Ch);
	_Self& assign(const_iterator _First, const_iterator _Last);

	value_type at(size_type index) const;

	const_iterator begin() const { return m_str; };
	const_iterator end() const { return m_str + length(); };


	//! cast to C string operator.
	operator const_str() const { return m_str; }

	//! cast to C string.
	const value_type *c_str() const { return m_str; }
	const value_type *data() const { return m_str; };

	//////////////////////////////////////////////////////////////////////////
	// string comparison.
	//////////////////////////////////////////////////////////////////////////
	int compare(const _Self& _Str) const;
	int compare(size_type _Pos1, size_type _Num1, const _Self& _Str) const;
	int compare(size_type _Pos1, size_type _Num1, const _Self& _Str, size_type nOff, size_type nCount) const;
	int compare(const value_type* _Ptr) const;
	int compare(size_type _Pos1, size_type _Num1, const value_type* _Ptr) const;
	int compare(size_type _Pos1, size_type _Num1, const value_type* _Ptr, size_type _Num2 = npos) const;

	// Case insensitive comparison
	int compareNoCase(const _Self& _Str) const;
	int compareNoCase(size_type _Pos1, size_type _Num1, const _Self& _Str) const;
	int compareNoCase(size_type _Pos1, size_type _Num1, const _Self& _Str, size_type nOff, size_type nCount) const;
	int compareNoCase(const value_type* _Ptr) const;
	int compareNoCase(size_type _Pos1, size_type _Num1, const value_type* _Ptr) const;
	int compareNoCase(size_type _Pos1, size_type _Num1, const value_type* _Ptr, size_type _Num2 = npos) const;

	// Copies at most a specified number of characters from an indexed position in a source string to a target character array.
	size_type copy(value_type* _Ptr, size_type nCount, size_type nOff = 0) const;

	void push_back(value_type _Ch) { _ConcatenateInPlace(&_Ch, 1); }
	void resize(size_type nCount, value_type _Ch = ' ');

	//! simple sub-string extraction
	_Self substr(size_type pos, size_type count = npos) const;

	// replace part of string.
	_Self& replace(value_type chOld, value_type chNew);
	_Self& replace(const_str strOld, const_str strNew);
	_Self& replace(size_type pos, size_type count, const_str strNew);
	_Self& replace(size_type pos, size_type count, const_str strNew, size_type count2);
	_Self& replace(size_type pos, size_type count, size_type nNumChars, value_type chNew);

	// insert new elements to string.
	_Self& insert(size_type nIndex, value_type ch);
	_Self& insert(size_type nIndex, size_type nCount, value_type ch);
	_Self& insert(size_type nIndex, const_str pstr);
	_Self& insert(size_type nIndex, const_str pstr, size_type nCount);

	//! delete count characters starting at zero-based index
	_Self& erase(size_type nIndex, size_type count = npos);

	//! searching (return starting index, or -1 if not found)
	//! look for a single character match
	//! like "C" strchr
	size_type find(value_type ch, size_type pos = 0) const;
	//! look for a specific sub-string
	//! like "C" strstr
	size_type find(const_str subs, size_type pos = 0) const;
	size_type rfind(value_type ch, size_type pos = npos) const;
	size_type rfind(const _Self& subs, size_type pos = 0) const;

	size_type find_first_of(value_type _Ch, size_type nOff = 0) const;
	size_type find_first_of(const_str charSet, size_type nOff = 0) const;
	size_type find_first_of(const _Self& _Str, size_type _Off = 0) const;

	size_type find_first_not_of(value_type _Ch, size_type _Off = 0) const;
	size_type find_first_not_of(const value_type* _Ptr, size_type _Off = 0) const;
	size_type find_first_not_of(const value_type* _Ptr, size_type _Off, size_type _Count) const;
	size_type find_first_not_of(const _Self& _Str, size_type _Off = 0) const;

	size_type find_last_of(value_type _Ch, size_type _Off = npos) const;
	size_type find_last_of(const value_type* _Ptr, size_type _Off = npos) const;
	size_type find_last_of(const value_type* _Ptr, size_type _Off, size_type _Count) const;
	size_type find_last_of(const _Self& _Str, size_type _Off = npos) const;

	size_type find_last_not_of(value_type _Ch, size_type _Off = npos) const;
	size_type find_last_not_of(const value_type* _Ptr, size_type _Off = npos) const;
	size_type find_last_not_of(const value_type* _Ptr, size_type _Off, size_type _Count) const;
	size_type find_last_not_of(const _Self& _Str, size_type _Off = npos) const;


	void swap(_Self& _Str);

	//////////////////////////////////////////////////////////////////////////
	// overloaded operators.
	//////////////////////////////////////////////////////////////////////////
	// overloaded indexing.
	//value_type operator[]( size_type index ) const; // same as at()
	//	value_type&	operator[]( size_type index ); // same as at()

	// overloaded assignment
	_Self& operator=(const _Self& str);
	_Self& operator=(value_type ch);
	_Self& operator=(const_str str);

	template<size_t AnySize> 
	StrRef(const xStrRefStack<T, AnySize> & str);

protected:
	// we prohibit an implicit conversion from CryStackString to make user aware of allocation!
	// -> use string(stackedString) instead
	// as the private statement seems to be ignored (VS C++), we add a compile time error, see below
	template<size_t AnySize> _Self& operator=(const xStrRefStack<T, AnySize> & str)
	{
		// we add a compile-time error as the Visual C++ compiler seems to ignore the private statement?
		STATIC_CHECK(0, Use_Explicit_String_Assignment_When_Assigning_From_StackString);
		// not reached, as above will generate a compile time error
		_Assign(str.c_str(), str.length());
		return *this;
	}

public:
	// string concatenation
	_Self& operator+=(const _Self& str);
	_Self& operator+=(value_type ch);
	_Self& operator+=(const_str str);

	//template <class TT> friend StrRef<TT> operator+( const StrRef<TT>& str1, const StrRef<TT>& str2 );
	//template <class TT> friend StrRef<TT> operator+( const StrRef<TT>& str, value_type ch );
	//template <class TT> friend StrRef<TT> operator+( value_type ch, const StrRef<TT>& str );
	//template <class TT> friend StrRef<TT> operator+( const StrRef<TT>& str1, const_str str2 );
	//template <class TT> friend StrRef<TT> operator+( const_str str1, const StrRef<TT>& str2 );

	size_t GetAllocatedMemory() const
	{
		StrHeader* header = _header();
		if (header == _emptyHeader())
			return 0;
		return sizeof(StrHeader)+(header->nAllocSize + 1)*sizeof(value_type);
	}

	//////////////////////////////////////////////////////////////////////////
	// Extended functions.
	// This functions are not in the STL string.
	// They have an ATL CString interface.
	//////////////////////////////////////////////////////////////////////////
	//! Format string, use (sprintf)
	_Self& Format(const value_type* format, ...);
	// This is _fast_ version
	_Self& MakeLower();
	// This is correct version
	_Self& MakeLowerLocale();
	_Self& MakeUpper();

	_Self& Trim();
	_Self& Trim(value_type ch);
	_Self& Trim(const value_type *sCharSet);

	_Self& TrimLeft();
	_Self& TrimLeft(value_type ch);
	_Self& TrimLeft(const value_type *sCharSet);

	_Self& TrimRight();
	_Self& TrimRight(value_type ch);
	_Self& TrimRight(const value_type *sCharSet);

	_Self SpanIncluding(const_str charSet) const;
	_Self SpanExcluding(const_str charSet) const;
	_Self Tokenize(const_str charSet, int &nStart) const;
	_Self Mid(size_type nFirst, size_type nCount = npos) const { return substr(nFirst, nCount); };

	_Self Left(size_type count) const;
	_Self Right(size_type count) const;
	//////////////////////////////////////////////////////////////////////////

	// public utilities.
	static size_type _strlen(const_str str);
	static size_type _strnlen(const_str str, size_type maxLen);
	static const_str _strchr(const_str str, value_type c);
	static value_type* _strstr(value_type* str, const_str strSearch);

#if defined(WIN32) || defined(WIN64)
	static int _vscpf(const_str format, va_list args);
#endif
	static int _vsnpf(value_type* buf, int cnt, const_str format, va_list args);

public:


protected:
	value_type* m_str; // pointer to ref counted string data

	// String header. Immediately after this header in memory starts actual string data.
	struct StrHeader
	{
		int nRefCount;
		int nLength;
		int nAllocSize;		// Size of memory allocated at the end of this class.

		value_type* GetChars() { return (value_type*)(this + 1); }
		void  AddRef() { nRefCount++;  };
		int   Release() { return --nRefCount; };
	};
	static StrHeader* _emptyHeader()
	{
		// Define 2 static buffers in a row. The 2nd is a dummy object to hold a single empty char string.
		static StrHeader sEmptyStringBuffer[2] = { { -1, 0, 0 }, { 0, 0, 0 } };
		return &sEmptyStringBuffer[0];
	}

	// implementation helpers
	StrHeader* _header() const;

	void _AllocData(size_type nLen);
	static void _FreeData(StrHeader* pData);
	void _Free();
	void _Initialize();

	void _Concatenate(const_str sStr1, size_type nLen1, const_str sStr2, size_type nLen2);
	void _ConcatenateInPlace(const_str sStr, size_type nLen);
	void _Assign(const_str sStr, size_type nLen);
	void _MakeUnique();

	static void _copy(value_type *dest, const value_type *src, size_type count);
	static void _move(value_type *dest, const value_type *src, size_type count);
	static void _set(value_type *dest, value_type ch, size_type count);
};



class ConstCharWrapper
{
public:
	//passing *this is safe since the char pointer is already set and therefore is the this-ptr constructed complete enough
#pragma warning (disable : 4355) 
	ConstCharWrapper(const char *const cpString) : // create stack string
		cpChar(cpString),
		str(*this)
	{
		//	X_ASSERT_NOT_NULL(cpString)(); 
	}
#pragma warning (default : 4355) 

	~ConstCharWrapper() {
		str.m_str = StrRef<char>::_emptyHeader()->GetChars();
	} //reset string

	operator const StrRef<char>&() const { return str; }//cast operator to const string reference 

private:
	const char *const cpChar;
	StrRef<char> str;

	char* GetCharPointer() const { return const_cast<char*>(cpChar); }	//access function for string ctor

	friend class StrRef<char>;	//both are bidirectional friends to avoid any other accesses

	X_NO_ASSIGN(ConstCharWrapper);
};


#define X_CONST_STRING(a) ((const string&)ConstCharWrapper(a))	



//////////////////////////////////////////////////////////////////////////

template <class T>
inline typename StrRef<T>::StrHeader* StrRef<T>::_header() const
{
	X_ASSERT_NOT_NULL(m_str);

	return ((StrHeader*)m_str) - 1;
}

template <class T>
inline typename StrRef<T>::size_type StrRef<T>::_strlen(const_str str)
{
	return (str == NULL) ? 0 : (size_type)core::strUtil::strlen(str);
}

template <>
inline StrRef<wchar_t>::size_type StrRef<wchar_t>::_strlen(const_str str)
{
	return (str == NULL) ? 0 : (size_type)::wcslen(str);
}

template <class T>
inline typename StrRef<T>::size_type StrRef<T>::_strnlen(const_str str, size_type maxLen)
{
	size_type len = 0;
	if (str)
	{
		while (str[0] && len<maxLen)
		{
			len++;
			str++;
		}
	}
	return len;
}

template <class T>
inline typename StrRef<T>::const_str StrRef<T>::_strchr(const_str str, value_type c)
{
	return (str == NULL) ? 0 : ::strchr(str, c);
}

template <>
inline StrRef<wchar_t>::const_str StrRef<wchar_t>::_strchr(const_str str, value_type c)
{
	return (str == NULL) ? 0 : ::wcschr(str, c);
}

template <class T>
inline typename StrRef<T>::value_type* StrRef<T>::_strstr(value_type* str, const_str strSearch)
{
	return (str == NULL) ? 0 : (value_type*)::strstr(str, strSearch);
}

template <>
inline StrRef<wchar_t>::value_type* StrRef<wchar_t>::_strstr(value_type* str, const_str strSearch)
{
	return (str == NULL) ? 0 : ::wcsstr(str, strSearch);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void StrRef<T>::_copy(value_type *dest, const value_type *src, size_type count)
{
	if (dest != src)
		memcpy(dest, src, count*sizeof(value_type));
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void StrRef<T>::_move(value_type *dest, const value_type *src, size_type count)
{
	memmove(dest, src, count*sizeof(value_type));
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void StrRef<T>::_set(value_type *dest, value_type ch, size_type count)
{
	memset(dest, ch, count*sizeof(value_type));
}

//////////////////////////////////////////////////////////////////////////
template <>
inline void StrRef<wchar_t>::_set(value_type *dest, value_type ch, size_type count)
{
	wmemset(dest, ch, count);
}

#if defined(WIN32) || defined(WIN64)

template<>
inline int StrRef<char>::_vscpf(const_str format, va_list args)
{
	return _vscprintf(format, args);
}

template<>
inline int StrRef<wchar_t>::_vscpf(const_str format, va_list args)
{
	return _vscwprintf(format, args);
}

template<>
inline int StrRef<char>::_vsnpf(value_type* buf, int cnt, const_str format, va_list args)
{
	return _vsnprintf(buf, (size_t)cnt, format, args);
}

template<>
inline int StrRef<wchar_t>::_vsnpf(value_type* buf, int cnt, const_str format, va_list args)
{
	return _vsnwprintf(buf, (size_t)cnt, format, args);
}

#else

template<>
inline int StrRef<char>::_vsnpf(value_type* buf, int cnt, const_str format, va_list args)
{
	return vsnprintf(buf, cnt, format, args);
}

template<>
inline int StrRef<wchar_t>::_vsnpf(value_type* buf, int cnt, const_str format, va_list args)
{
	return vswprintf(buf, cnt, format, args);
}

#endif

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
template <class T>
inline void StrRef<T>::_Assign(const_str sStr, size_type Len)
{
	// Check if this string is shared (reference count greater then 1) or not enough capacity to store new string.
	// Then allocate new string buffer.
	if (_header()->nRefCount > 1 || Len > capacity())
	{
		_Free();
		_AllocData(Len);
	}
	// Copy characters from new string to this buffer.
	_copy(m_str, sStr, Len);
	// Set new length.
	_header()->nLength = safe_static_cast<int, size_t>(Len);
	// Make null terminated string.
	m_str[Len] = 0;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void StrRef<T>::_Concatenate(const_str sStr1, size_type nLen1, const_str sStr2, size_type nLen2)
{
	size_type nLen = nLen1 + nLen2;
#if !defined(PS3) && !defined(XENON)
	if (nLen1 * 2 > nLen)
		nLen = nLen1 * 2;
#endif
	if (nLen != 0)
	{
#if !defined(PS3) && !defined(XENON)
		if (nLen < 8)
			nLen = 8;
#endif
		_AllocData(nLen);
		_copy(m_str, sStr1, nLen1);
		_copy(m_str + nLen1, sStr2, nLen2);
		check_convert(_header()->nLength) = nLen1 + nLen2;
		m_str[nLen1 + nLen2] = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void StrRef<T>::_ConcatenateInPlace(const_str sStr, size_type nLen)
{
	if (nLen != 0)
	{
		// Check if this string is shared (reference count greater then 1) or not enough capacity to store new string.
		// Then allocate new string buffer.
		if (_header()->nRefCount > 1 || length() + nLen > capacity())
		{
			StrHeader* pOldData = _header();
			_Concatenate(m_str, length(), sStr, nLen);
			_FreeData(pOldData);
		}
		else
		{
			_copy(m_str + length(), sStr, nLen);
			check_convert(_header()->nLength) = _header()->nLength + nLen;
			m_str[_header()->nLength] = 0; // Make null terminated string.
		}
	}
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void StrRef<T>::_MakeUnique()
{
	if (_header()->nRefCount > 1)
	{
		// If string is shared, make a copy of string buffer.
		StrHeader *pOldData = _header();
		// This will not free header because reference count is greater then 1.
		_Free();
		// Allocate a new string buffer.
		_AllocData(pOldData->nLength);
		// Full copy of null terminated string.
		_copy(m_str, pOldData->GetChars(), pOldData->nLength + 1);
	}
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void StrRef<T>::_Initialize()
{
	m_str = _emptyHeader()->GetChars();
}

// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
template <class T>
inline void StrRef<T>::_AllocData(size_type Len)
{
	X_ASSERT(Len >= 0 && Len <= (INT_MAX - 1), "length is invalid")(Len);

	if (Len == 0)
		_Initialize();
	else
	{
		size_type allocLen = sizeof(StrHeader)+(Len + 1)*sizeof(value_type);

#if 0
#if defined(NOT_USE_CRY_MEMORY_MANAGER)
		StrHeader* pData = (StrHeader*)CryModuleMalloc(allocLen);
#else
		StrHeader* pData = (StrHeader*)CryModuleMalloc(allocLen, eCryM_Launcher);
#endif
#else
		StrHeader* pData = (StrHeader*)new BYTE[allocLen];
#endif


		pData->nRefCount = 1;
		m_str = pData->GetChars();
		pData->nLength = safe_static_cast<int, size_t>(Len);
		pData->nAllocSize = safe_static_cast<int, size_t>(Len);
		m_str[Len] = 0; // null terminated string.
	}
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void StrRef<T>::_Free()
{
	if (_header()->nRefCount >= 0) // Not empty string.
	{
		_FreeData(_header());
		_Initialize();
	}
}

//////////////////////////////////////////////////////////////////////////

template <class T>
inline void StrRef<T>::_FreeData(StrHeader* pData)
{
	if (pData->nRefCount >= 0) // Not empty string.
	{
		X_ASSERT(pData->nRefCount != 0, "invalid ref count")();
		if (pData->Release() <= 0)
		{
			//	size_t allocLen = sizeof(StrHeader)+(pData->nAllocSize + 1)*sizeof(value_type);

			delete[] pData;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>::StrRef()
{
	_Initialize();
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>::StrRef(const StrRef<T>& str)
{
	X_ASSERT(str._header()->nRefCount != 0, "invalid ref count")();
	if (str._header()->nRefCount >= 0)
	{
		m_str = str.m_str;
		_header()->AddRef();
	}
	else
	{
		_Initialize();
	}
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>::StrRef(const StrRef<T>& str, size_type nOff, size_type nCount)
{
	_Initialize();
	assign(str, nOff, nCount);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>::StrRef(const_str str)
{
	_Initialize();
	// Make a copy of C string.
	size_type nLen = _strlen(str);
	if (nLen != 0)
	{
		_AllocData(nLen);
		_copy(m_str, str, nLen);
	}
}

template <class T>
inline StrRef<T>::StrRef(const ConstCharWrapper& str)
{
	_Initialize();
	m_str = const_cast<pointer>(str.GetCharPointer());
}

template<class T>
template<size_t AnySize>
inline StrRef<T>::StrRef(const xStrRefStack<T, AnySize> & str)
{
	_Initialize();
	const size_type nLength = str.length();
	if (nLength > 0)
	{
		_AllocData(nLength);
		_copy(m_str, str, nLength);
	}
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>::StrRef(const_str str, size_type nLength)
{
	_Initialize();
	if (nLength > 0)
	{
		_AllocData(nLength);
		_copy(m_str, str, nLength);
	}
}


//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>::StrRef(value_type ch, size_type nRepeat)
{
	_Initialize();
	if (nRepeat > 0)
	{
		_AllocData(nRepeat);
		_set(m_str, ch, nRepeat);
	}
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>::StrRef(const_iterator _First, const_iterator _Last)
{
	_Initialize();
	size_type nLength = (size_type)(_Last - _First);
	if (nLength > 0)
	{
		_AllocData(nLength);
		_copy(m_str, _First, nLength);
	}
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>::~StrRef()
{
	_FreeData(_header());
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::length() const
{
	return _header()->nLength;
}
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::size() const
{
	return _header()->nLength;
}
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::capacity() const
{
	return _header()->nAllocSize;
}

template <class T>
inline bool StrRef<T>::empty() const
{
	return length() == 0;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void StrRef<T>::clear()
{
	if (length() == 0)
		return;
	if (_header()->nRefCount >= 0)
		_Free();
	else
		resize(0);
	X_ASSERT(length() == 0, "Failed to clear")(length());
	X_ASSERT(_header()->nRefCount < 0 || capacity() == 0, "")();
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void StrRef<T>::reserve(size_type nCount)
{
	// Reserve of 0 is shrinking container to fit number of characters in it..
	if (nCount > capacity())
	{
		StrHeader* pOldData = _header();
		_AllocData(nCount);
		_copy(m_str, pOldData->GetChars(), pOldData->nLength);
		_header()->nLength = pOldData->nLength;
		m_str[pOldData->nLength] = 0;
		_FreeData(pOldData);
	}
	else if (nCount == 0)
	{
		if (length() != capacity())
		{
			StrHeader* pOldData = _header();
			_AllocData(length());
			_copy(m_str, pOldData->GetChars(), pOldData->nLength);
			_FreeData(pOldData);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::append(const_str _Ptr)
{
	*this += _Ptr;
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::append(const_str _Ptr, size_type nCount)
{
	_ConcatenateInPlace(_Ptr, nCount);
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::append(const StrRef<T>& _Str, size_type off, size_type nCount)
{
	size_type len = _Str.length();
	if (off > len)
		return *this;
	if (off + nCount > len)
		nCount = len - off;
	_ConcatenateInPlace(_Str.m_str + off, nCount);
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::append(const StrRef<T>& _Str)
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
	CheckConvert(D& d)
	: dest(&d) {}

	template<class S>
	D& operator=(S const& s)
	{
		return check_convert(*dest, s);
	}

protected:
	D*	dest;
};

template<class D>
inline CheckConvert<D> check_convert(D& d)
{
	return d;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::append(size_type nCount, value_type _Ch)
{
	if (nCount > 0)
	{
		if (length() + nCount >= capacity())
		{
			StrHeader* pOldData = _header();
			_AllocData(length() + nCount);
			_copy(m_str, pOldData->GetChars(), pOldData->nLength);
			_set(m_str + pOldData->nLength, _Ch, nCount);
			_FreeData(pOldData);
		}
		else
		{
			size_type nOldLength = length();
			_set(m_str + nOldLength, _Ch, nCount);
			check_convert(_header()->nLength) = nOldLength + nCount;
			m_str[length()] = 0; // Make null terminated string.
		}
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::append(const_iterator _First, const_iterator _Last)
{
	append(_First, (size_type)(_Last - _First));
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::assign(const_str _Ptr)
{
	*this = _Ptr;
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::assign(const_str _Ptr, size_type nCount)
{
	size_type len = _strnlen(_Ptr, nCount);
	_Assign(_Ptr, len);
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::assign(const StrRef<T>& _Str, size_type off, size_type nCount)
{
	size_type len = _Str.length();
	if (off > len)
		return *this;
	if (off + nCount > len)
		nCount = len - off;
	_Assign(_Str.m_str + off, nCount);
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::assign(const StrRef<T>& _Str)
{
	*this = _Str;
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::assign(size_type nCount, value_type _Ch)
{
	if (nCount >= 1)
	{
		_AllocData(nCount);
		_set(m_str, _Ch, nCount);
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::assign(const_iterator _First, const_iterator _Last)
{
	assign(_First, (size_type)(_Last - _First));
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::value_type StrRef<T>::at(size_type index) const
{
	X_ASSERT(index >= 0 && index < length(), "index out of range")(index, length());

	return m_str[index];
}


//////////////////////////////////////////////////////////////////////////
template <class T>
inline int StrRef<T>::compare(const StrRef<T>& _Str) const
{
	return strcmp(m_str, _Str.m_str);
}

template <>
inline int StrRef<wchar_t>::compare(const StrRef<wchar_t>& _Str) const
{
	return wcscmp(m_str, _Str.m_str);
}

template <class T>
inline int StrRef<T>::compare(size_type _Pos1, size_type _Num1, const StrRef<T>& _Str) const
{
	return compare(_Pos1, _Num1, _Str.m_str, npos);
}

template <class T>
inline int StrRef<T>::compare(size_type _Pos1, size_type _Num1, const StrRef<T>& _Str, size_type Off, size_type nCount) const
{
	X_ASSERT(Off < _Str.length(), "shit went wronge lol")(nOff);

	return compare(_Pos1, _Num1, _Str.m_str + Off, nCount);
}

template <class T>
inline int StrRef<T>::compare(const value_type* _Ptr) const
{
	return strcmp(m_str, _Ptr);
}

template <>
inline int StrRef<wchar_t>::compare(const wchar_t* _Ptr) const
{
	return wcscmp(m_str, _Ptr);
}


template <class T>
inline int StrRef<T>::compare(size_type _Pos1, size_type _Num1, const value_type* _Ptr) const
{
	return compare(_Pos1, _Num1, _Ptr, npos);
}

template <class T>
inline int StrRef<T>::compare(size_type Pos, size_type _Num1, const value_type* _Ptr, size_type _Num2) const
{
	X_ASSERT(Pos < length(), "shit went wronge lol")(Pos, length());


	if (length() - Pos < _Num1)
		_Num1 = length() - Pos; // trim to size

	int res = _Num1 == 0 ? 0 : strncmp(m_str + Pos, _Ptr, (_Num1 < _Num2) ? _Num1 : _Num2);
	return (res != 0 ? res : _Num1 < _Num2 ? -1 : _Num1 == _Num2 ? 0 : +1);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline int StrRef<T>::compareNoCase(const StrRef<T>& _Str) const
{
	return stricmp(m_str, _Str.m_str);
}

template <>
inline int StrRef<wchar_t>::compareNoCase(const StrRef<wchar_t>& _Str) const
{
	return _wcsicmp(m_str, _Str.m_str);
}

template <class T>
inline int StrRef<T>::compareNoCase(size_type _Pos1, size_type _Num1, const StrRef<T>& _Str) const
{
	return compareNoCase(_Pos1, _Num1, _Str.m_str, npos);
}

template <class T>
inline int StrRef<T>::compareNoCase(size_type _Pos1, size_type _Num1, const StrRef<T>& _Str, size_type nOff, size_type nCount) const
{
	X_ASSERT(nOff < _Str.length(), "index out of range")(nOff, _Str.length());
	return compareNoCase(_Pos1, _Num1, _Str.m_str + nOff, nCount);
}

template <class T>
inline int StrRef<T>::compareNoCase(const value_type* _Ptr) const
{
	return _stricmp(m_str, _Ptr);
}

template <>
inline int StrRef<wchar_t>::compareNoCase(const wchar_t* _Ptr) const
{
	return _wcsicmp(m_str, _Ptr);
}

template <class T>
inline int StrRef<T>::compareNoCase(size_type _Pos1, size_type _Num1, const value_type* _Ptr) const
{
	return compareNoCase(_Pos1, _Num1, _Ptr, npos);
}

template <class T>
inline int StrRef<T>::compareNoCase(size_type Pos, size_type _Num1, const value_type* _Ptr, size_type _Num2) const
{
	X_ASSERT(Pos < length(), "shit went wronge lol")(Pos, length());


	if (length() - Pos < _Num1)
		_Num1 = length() - Pos; // trim to size

	int res = _Num1 == 0 ? 0 : strnicmp(m_str + Pos, _Ptr, (_Num1 < _Num2) ? _Num1 : _Num2);
	return (res != 0 ? res : _Num1 < _Num2 ? -1 : _Num1 == _Num2 ? 0 : +1);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::copy(value_type* _Ptr, size_type nCount, size_type Off) const
{
	X_ASSERT(Off < length(), "shit went wronge lol")(Off, length());

	if (nCount < 0)
		nCount = 0;
	if (Off + nCount > length()) // trim to offset.
		nCount = length() - Off;

	_copy(_Ptr, m_str + Off, nCount);
	return nCount;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void StrRef<T>::resize(size_type nCount, value_type _Ch)
{
	_MakeUnique();
	if (nCount > length())
	{
		size_type numToAdd = nCount - length();
		append(numToAdd, _Ch);
	}
	else if (nCount < length())
	{
		_header()->nLength = safe_static_cast<int, size_t>(nCount);
		m_str[length()] = 0; // Make null terminated string.

	}
}

//////////////////////////////////////////////////////////////////////////
//! compare helpers
template <class T> inline bool operator==(const StrRef<T>& s1, const StrRef<T>& s2)
{
	return s1.compare(s2) == 0;
}
template <class T> inline bool operator==(const StrRef<T>& s1, const typename StrRef<T>::value_type* s2)
{
	return s1.compare(s2) == 0;
}
template <class T> inline bool operator==(const typename StrRef<T>::value_type* s1, const StrRef<T>& s2)
{
	return s2.compare(s1) == 0;
}
template <class T> inline bool operator!=(const StrRef<T>& s1, const StrRef<T>& s2)
{
	return s1.compare(s2) != 0;
}
template <class T> inline bool operator!=(const StrRef<T>& s1, const typename StrRef<T>::value_type* s2)
{
	return s1.compare(s2) != 0;
}
template <class T> inline bool operator!=(const typename StrRef<T>::value_type* s1, const StrRef<T>& s2)
{
	return s2.compare(s1) != 0;
}
template <class T> inline bool operator<(const StrRef<T>& s1, const StrRef<T>& s2)
{
	return s1.compare(s2) < 0;
}
template <class T> inline bool operator<(const StrRef<T>& s1, const typename StrRef<T>::value_type* s2)
{
	return s1.compare(s2) < 0;
}
template <class T> inline bool operator<(const typename StrRef<T>::value_type* s1, const StrRef<T>& s2)
{
	return s2.compare(s1) > 0;
}
template <class T> inline bool operator>(const StrRef<T>& s1, const StrRef<T>& s2)
{
	return s1.compare(s2) > 0;
}
template <class T> inline bool operator>(const StrRef<T>& s1, const typename StrRef<T>::value_type* s2)
{
	return s1.compare(s2) > 0;
}
template <class T> inline bool operator>(const typename StrRef<T>::value_type* s1, const StrRef<T>& s2)
{
	return s2.compare(s1) < 0;
}
template <class T> inline bool operator<=(const StrRef<T>& s1, const StrRef<T>& s2)
{
	return s1.compare(s2) <= 0;
}
template <class T> inline bool operator<=(const StrRef<T>& s1, const typename StrRef<T>::value_type* s2)
{
	return s1.compare(s2) <= 0;
}
template <class T> inline bool operator<=(const typename StrRef<T>::value_type* s1, const StrRef<T>& s2)
{
	return s2.compare(s1) >= 0;
}
template <class T> inline bool operator>=(const StrRef<T>& s1, const StrRef<T>& s2)
{
	return s1.compare(s2) >= 0;
}
template <class T> inline bool operator>=(const StrRef<T>& s1, const typename StrRef<T>::value_type* s2)
{
	return s1.compare(s2) >= 0;
}
template <class T> inline bool operator>=(const typename StrRef<T>::value_type* s1, const StrRef<T>& s2)
{
	return s2.compare(s1) <= 0;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::operator=(value_type ch)
{
	_Assign(&ch, 1);
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T> operator+(const StrRef<T>& string1, typename StrRef<T>::value_type ch)
{
	StrRef<T> s(string1);
	s.append(1, ch);
	return s;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T> operator+(typename StrRef<T>::value_type ch, const StrRef<T>& str)
{
	StrRef<T> s;
	s.reserve(str.size() + 1);
	s.append(1, ch);
	s.append(str);
	return s;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T> operator+(const StrRef<T>& string1, const StrRef<T>& string2)
{
	StrRef<T> s(string1);
	s.append(string2);
	return s;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T> operator+(const StrRef<T>& str1, const typename StrRef<T>::value_type* str2)
{
	StrRef<T> s(str1);
	s.append(str2);
	return s;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T> operator+(const typename StrRef<T>::value_type* str1, const StrRef<T>& str2)
{
	X_ASSERT_NOT_NULL(str);

	StrRef<T> s;
	s.reserve(StrRef<T>::_strlen(str1) + str2.size());
	s.append(str1);
	s.append(str2);
	return s;
}

template <class T>
inline StrRef<T>& StrRef<T>::operator=(const StrRef<T>& str)
{
	if (m_str != str.m_str)
	{
		if (_header()->nRefCount < 0)
		{
			// Empty string.
			//			_Assign( str.m_str,str.length() );
			if (str._header()->nRefCount < 0)
				; // two empty strings...
			else
			{
				m_str = str.m_str;
				_header()->AddRef();
			}
		}
		else if (str._header()->nRefCount < 0)
		{
			// If source string is empty.
			_Free();
			m_str = str.m_str;
		}
		else
		{
			// Copy string reference.
			_Free();
			m_str = str.m_str;
			_header()->AddRef();
		}
	}
	return *this;
}

template <class T>
inline StrRef<T>& StrRef<T>::operator=(const_str str)
{
	X_ASSERT_NOT_NULL(str);

	_Assign(str, _strlen(str));
	return *this;
}

template <class T>
inline StrRef<T>& StrRef<T>::operator+=(const_str str)
{
	X_ASSERT_NOT_NULL(str);

	_ConcatenateInPlace(str, _strlen(str));
	return *this;
}

template <class T>
inline StrRef<T>& StrRef<T>::operator+=(value_type ch)
{
	_ConcatenateInPlace(&ch, 1);
	return *this;
}

template <class T>
inline StrRef<T>& StrRef<T>::operator+=(const StrRef<T>& str)
{
	_ConcatenateInPlace(str.m_str, str.length());
	return *this;
}

//! find first single character
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find(value_type ch, size_type pos) const
{
	if (!(pos >= 0 && pos <= length()))
		return (typename StrRef<T>::size_type)npos;
	const_str str = _strchr(m_str + pos, ch);
	// return npos if not found and index otherwise
	return (str == NULL) ? npos : (size_type)(str - m_str);
}

//! find a sub-string (like strstr)
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find(const_str subs, size_type pos) const
{
	if (!(pos >= 0 && pos <= length()))
		return npos;

	// find first matching substring
	const_str str = _strstr(m_str + pos, subs);

	// return npos for not found, distance from beginning otherwise
	return (str == NULL) ? npos : (size_type)(str - m_str);

}

//! find last single character
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::rfind(value_type ch, size_type pos) const
{
	const_str str;
	if (pos == npos) {
		// find last single character
		str = strrchr(m_str, ch);
		// return -1 if not found, distance from beginning otherwise
		return (str == NULL) ? (size_type)-1 : (size_type)(str - m_str);
	}
	else {
		if (pos == npos)
			pos = length();
		if (!(pos >= 0 && pos <= length()))
			return npos;

		value_type tmp = m_str[pos + 1];
		m_str[pos + 1] = 0;
		str = strrchr(m_str, ch);
		m_str[pos + 1] = tmp;
	}
	// return -1 if not found, distance from beginning otherwise
	return (str == NULL) ? (size_type)-1 : (size_type)(str - m_str);
}

template <class T>
inline typename StrRef<T>::size_type StrRef<T>::rfind(const StrRef<T>& subs, size_type pos) const
{
	size_type res = npos;
	for (int i = (int)size(); i >= (int)pos; --i)
	{
		size_type findRes = find(subs, i);
		if (findRes != npos)
		{
			res = findRes;
			break;
		}
	}
	return res;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_first_of(const StrRef<T>& _Str, size_type _Off) const
{
	return find_first_of(_Str.m_str, _Off);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_first_of(value_type _Ch, size_type nOff) const
{
	if (!(nOff >= 0 && nOff <= length()))
		return npos;
	value_type charSet[2] = { _Ch, 0 };
	const_str str = strpbrk(m_str + nOff, charSet);
	return (str == NULL) ? (size_type)-1 : (size_type)(str - m_str);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_first_of(const_str charSet, size_type nOff) const
{
	if (!(nOff >= 0 && nOff <= length()))
		return npos;
	const_str str = strpbrk(m_str + nOff, charSet);
	return (str == NULL) ? (size_type)-1 : (size_type)(str - m_str);
}



//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_first_not_of(const value_type* _Ptr, size_type _Off) const
{
	return find_first_not_of(_Ptr, _Off, _strlen(_Ptr));
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_first_not_of(const StrRef<T>& _Str, size_type _Off) const
{
	return find_first_not_of(_Str.m_str, _Off);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_first_not_of(value_type _Ch, size_type _Off) const
{
	if (_Off > length())
		return npos;
	else {
		for (const value_type *str = begin() + _Off; str != end(); ++str)
		if (*str != _Ch)
			return size_type(str - begin()); // Character found!
		return npos;
	}
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_first_not_of(const value_type* _Ptr, size_type _Off, size_type _Count) const
{
	if (_Off > length())
		return npos;
	else {
		const value_type *charsFirst = _Ptr, *charsLast = _Ptr + _Count;
		for (const value_type *str = begin() + _Off; str != end(); ++str)
		{
			const value_type *c;
			for (c = charsFirst; c != charsLast; ++c)
			{
				if (*c == *str)
					break;
			}
			if (c == charsLast)
				return size_type(str - begin());// Current character not in char set.
		}
		return npos;
	}
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_last_of(value_type _Ch, size_type _Off) const
{
	size_type nLenght(length());
	// Empty strings, always return npos (same semantic as std::string).
	if (nLenght == 0)
	{
		return npos;
	}

	// If the offset is is bigger than the size of the string
	if (_Off >= nLenght)
	{
		// We set it to the size of the string -1, so we will not do bad pointer operations nor we will
		// test the null terminating character.
		_Off = nLenght - 1;
	}

	// From the character at the offset position, going to to the direction of the first character.
	for (const value_type *str = begin() + _Off; true; --str)
	{
		// We found a character in the string which matches the input character.
		if (*str == _Ch)
		{
			return size_type(str - begin()); // Character found!
		}
		// If the next element will be begin()-1, then we should stop.
		if (str == begin())
		{
			break;
		}
	}

	// We found nothing.
	return npos;
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_last_of(const value_type* _Ptr, size_type _Off) const
{
	// This function is actually a convenience alias...
	// BTW: what will happeb if wchar_t is used here?
	return find_last_of(_Ptr, _Off, _strlen(_Ptr));
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_last_of(const value_type* _Ptr, size_type _Off, size_type _Count) const
{
	size_type nLenght(length());
	// Empty strings, always return npos (same semantic as std::string).
	if (nLenght == 0)
	{
		return npos;
	}

	// If the offset is is bigger than the size of the string
	if (_Off >= nLenght)
	{
		// We set it to the size of the string -1, so we will not do bad pointer operations nor we will
		// test the null terminating character.
		_Off = nLenght - 1;
	}

	// From the character at the offset position, going to to the direction of the first character.
	const value_type *charsFirst = _Ptr, *charsLast = _Ptr + _Count;
	for (const value_type *str = begin() + _Off; true; --str)
	{
		const value_type *c;
		// For every character in the character set.
		for (c = charsFirst; c != charsLast; ++c)
		{
			// If the current character matches any of the charcaters in the input string...
			if (*c == *str)
			{
				// This is the value we must return.
				return size_type(str - begin());
			}
		}

		// If the next element will be begin()-1, then we should stop.
		if (str == begin())
		{
			break;
		}
	}
	// We couldn't find any character of the input string in the current string.
	return npos;
}
/////////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_last_of(const _Self& strCharSet, size_type _Off) const
{
	size_type nLenght(length());
	// Empty strings, always return npos (same semantic as std::string).
	if (nLenght == 0)
	{
		return npos;
	}

	// If the offset is is bigger than the size of the string
	if (_Off >= nLenght)
	{
		// We set it to the size of the string -1, so we will not do bad pointer operations nor we will
		// test the null terminating character.
		_Off = nLenght - 1;
	}


	// From the character at the offset position, going to to the direction of the first character.
	for (const value_type *str = begin() + _Off; true; --str)
	{
		// We check every character of the input string.
		for (const value_type *strInputCharacter = strCharSet.begin(); strInputCharacter != strCharSet.end(); ++strInputCharacter)
		{
			// If any character matches.
			if (*str == *strInputCharacter)
			{
				// We return the position where we found it.
				return size_type(str - begin()); // Character found!
			}
		}
		// If the next element will be begin()-1, then we should stop.
		if (str == begin())
		{
			break;
		}
	}

	// As we couldn't find any matching character...we return the appropriate value.
	return npos;
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_last_not_of(value_type _Ch, size_type _Off) const
{
	size_type nLenght(length());
	// Empty strings, always return npos (same semantic as std::string).
	if (nLenght == 0)
	{
		return npos;
	}

	// If the offset is is bigger than the size of the string
	if (_Off >= nLenght)
	{
		// We set it to the size of the string -1, so we will not do bad pointer operations nor we will
		// test the null terminating character.
		_Off = nLenght - 1;
	}


	// From the character at the offset position, going to to the direction of the first character.
	for (const value_type *str = begin() + _Off; true; --str)
	{
		// If the current character being analyzed is different of the input character.
		if (*str != _Ch)
		{
			// We found the last item which is not the input character before the given offset.
			return size_type(str - begin()); // Character found!
		}
		// If the next element will be begin()-1, then we should stop.
		if (str == begin())
		{
			break;
		}
	}

	// As we couldn't find any matching character...we return the appropriate value.
	return npos;
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_last_not_of(const value_type* _Ptr, size_type _Off) const
{
	// This function is actually a convenience alias...
	// BTW: what will happeb if wchar_t is used here?
	return find_last_not_of(_Ptr, _Off, _strlen(_Ptr));
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_last_not_of(const value_type* _Ptr, size_type _Off, size_type _Count) const
{
	size_type nLenght(length());
	// Empty strings, always return npos (same semantic as std::string).
	if (nLenght == 0)
	{
		return npos;
	}

	// If the offset is is bigger than the size of the string
	if (_Off >= nLenght)
	{
		// We set it to the size of the string -1, so we will not do bad pointer operations nor we will
		// test the null terminating character.
		_Off = nLenght - 1;
	}


	// From the character at the offset position, going to to the direction of the first character.
	const value_type *charsFirst = _Ptr, *charsLast = _Ptr + _Count;
	for (const value_type *str = begin() + _Off; true; --str)
	{
		bool boFoundAny(false);
		const value_type *c;
		// For every character in the character set.
		for (c = charsFirst; c != charsLast; ++c)
		{
			// If the current character matches any of the charcaters in the input string...
			if (*c == *str)
			{
				// So we signal it was found and stop this search.
				boFoundAny = true;
				break;
			}
		}

		// Using a different solution of the other similar methods
		// to make it easier to read.
		// If the character being analyzed is not in the set...
		if (!boFoundAny)
		{
			//.. we return the position where we found it.
			return size_type(str - begin());
		}

		// If the next element will be begin()-1, then we should stop.
		if (str == begin())
		{
			break;
		}
	}
	// We couldn't find any character of the input string not in the character set.
	return npos;

}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline typename StrRef<T>::size_type StrRef<T>::find_last_not_of(const _Self& _Str, size_type _Off) const
{
	size_type nLenght(length());
	// Empty strings, always return npos (same semantic as std::string).
	if (nLenght == 0)
	{
		return npos;
	}

	// If the offset is is bigger than the size of the string
	if (_Off >= nLenght)
	{
		// We set it to the size of the string -1, so we will not do bad pointer operations nor we will
		// test the null terminating character.
		_Off = nLenght - 1;
	}


	// From the character at the offset position, going to to the direction of the first character.
	for (const value_type *str = begin() + _Off; true; --str)
	{
		bool boFoundAny(false);
		for (const value_type *strInputCharacter = _Str.begin(); strInputCharacter != _Str.end(); ++strInputCharacter)
		{
			// The character matched one of the character set...
			if (*strInputCharacter == *str)
			{
				// So we signal it was found and stop this search.
				boFoundAny = true;
				break;
			}
		}

		// Using a different solution of the other similar methods
		// to make it easier to read.
		// If the character being analyzed is not in the set...
		if (!boFoundAny)
		{
			//.. we return the position where we found it.
			return size_type(str - begin());
		}

		// If the next element will be begin()-1, then we should stop.
		if (str == begin())
		{
			break;
		}
	}

	// As we couldn't find any matching character...we return the appropriate value.
	return npos;
}
//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T> StrRef<T>::substr(size_type pos, size_type count) const
{
	if (pos >= length())
		return StrRef<T>();
	if (count == npos)
	{
		count = length() - pos;
	}
	if (pos + count > length())
		count = length() - pos;
	return StrRef<T>(m_str + pos, count);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::erase(size_type nIndex, size_type nCount)
{
	if (nIndex < 0)
		nIndex = 0;
	if (nCount < 0 || nCount > length() - nIndex)
		nCount = length() - nIndex;
	if (nCount > 0 && nIndex < length())
	{
		_MakeUnique();
		size_type nNumToCopy = length() - (nIndex + nCount) + 1;
		_move(m_str + nIndex, m_str + nIndex + nCount, nNumToCopy);
		_header()->nLength = safe_static_cast<int, size_t>(length() - nCount);
	}

	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::insert(size_type nIndex, value_type ch)
{
	return insert(nIndex, 1, ch);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::insert(size_type nIndex, size_type nCount, value_type ch)
{
	_MakeUnique();

	if (nIndex < 0)
		nIndex = 0;

	size_type nNewLength = length();
	if (nIndex > nNewLength)
		nIndex = nNewLength;
	nNewLength += nCount;

	if (capacity() < nNewLength)
	{
		StrHeader* pOldData = _header();
		const_str pstr = m_str;
		_AllocData(nNewLength);
		_copy(m_str, pstr, pOldData->nLength + 1);
		_FreeData(pOldData);
	}

	_move(m_str + nIndex + nCount, m_str + nIndex, (nNewLength - nIndex));
	_set(m_str + nIndex, ch, nCount);
	_header()->nLength = (int)nNewLength;

	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::insert(size_type nIndex, const_str pstr, size_type nCount)
{
	if (nIndex < 0)
		nIndex = 0;

	size_type nInsertLength = nCount;
	size_type nNewLength = length();
	if (nInsertLength > 0)
	{
		_MakeUnique();
		if (nIndex > nNewLength)
			nIndex = nNewLength;
		nNewLength += nInsertLength;

		if (capacity() < nNewLength)
		{
			StrHeader* pOldData = _header();
			const_str pOldStr = m_str;
			_AllocData(nNewLength);
			_copy(m_str, pOldStr, (pOldData->nLength + 1));
			_FreeData(pOldData);
		}

		_move(m_str + nIndex + nInsertLength, m_str + nIndex, (nNewLength - nIndex - nInsertLength + 1));
		_copy(m_str + nIndex, pstr, nInsertLength);
		_header()->nLength = safe_static_cast<int, size_type>(nNewLength);
		m_str[length()] = 0;
	}

	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::insert(size_type nIndex, const_str pstr)
{
	return insert(nIndex, pstr, _strlen(pstr));
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::replace(size_type pos, size_type count, const_str strNew)
{
	return replace(pos, count, strNew, _strlen(strNew));
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::replace(size_type pos, size_type count, const_str strNew, size_type count2)
{
	erase(pos, count);
	insert(pos, strNew, count2);
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::replace(size_type pos, size_type count, size_type nNumChars, value_type chNew)
{
	erase(pos, count);
	insert(pos, nNumChars, chNew);
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::replace(value_type chOld, value_type chNew)
{
	if (chOld != chNew)
	{
		_MakeUnique();
		value_type* strend = m_str + length();
		for (value_type* str = m_str; str != strend; ++str)
		{
			if (*str == chOld)
			{
				*str = chNew;
			}
		}
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::replace(const_str strOld, const_str strNew)
{
	size_type nSourceLen = _strlen(strOld);
	if (nSourceLen == 0)
		return *this;
	size_type nReplacementLen = _strlen(strNew);

	size_type nCount = 0;
	value_type* strStart = m_str;
	value_type* strEnd = m_str + length();
	value_type* strTarget;
	while (strStart < strEnd)
	{
		while ((strTarget = _strstr(strStart, strOld)) != NULL)
		{
			nCount++;
			strStart = strTarget + nSourceLen;
		}
		strStart += _strlen(strStart) + 1;
	}

	if (nCount > 0)
	{
		_MakeUnique();

		size_type nOldLength = length();
		size_type nNewLength = nOldLength + (nReplacementLen - nSourceLen)*nCount;
		if (capacity() < nNewLength || _header()->nRefCount > 1)
		{
			StrHeader* pOldData = _header();
			const_str pstr = m_str;
			_AllocData(nNewLength);
			_copy(m_str, pstr, pOldData->nLength);
			_FreeData(pOldData);
		}
		strStart = m_str;
		strEnd = m_str + length();

		while (strStart < strEnd)
		{
			while ((strTarget = _strstr(strStart, strOld)) != NULL)
			{
				size_type nBalance = nOldLength - ((size_type)(strTarget - m_str) + nSourceLen);
				_move(strTarget + nReplacementLen, strTarget + nSourceLen, nBalance);
				_copy(strTarget, strNew, nReplacementLen);
				strStart = strTarget + nReplacementLen;
				strStart[nBalance] = 0;
				nOldLength += (nReplacementLen - nSourceLen);
			}
			strStart += _strlen(strStart) + 1;
		}
		_header()->nLength = (int)nNewLength;
	}

	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline void StrRef<T>::swap(StrRef<T>& _Str)
{
	value_type *temp = _Str.m_str;
	_Str.m_str = m_str;
	m_str = temp;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::Format(const_str format, ...)
{
	va_list argList;
	va_start(argList, format);
	int n = _vscpf(format, argList);
	if (n < 0) n = 0;
	resize(n);
	_vsnpf(m_str, n, format, argList);
	va_end(argList);
	return *this;
}

//////////////////////////////////////////////////////////////////////////
#define __ascii_tolower(c)      ( (((c) >= 'A') && ((c) <= 'Z')) ? ((c) - 'A' + 'a') : (c) )
template <class T>
inline StrRef<T>& StrRef<T>::MakeLower()
{
	_MakeUnique();
	for (value_type *s = m_str; *s != 0; s++)
	{
		*s = __ascii_tolower(*s);
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::MakeLowerLocale()
{
	_MakeUnique();
	for (value_type *s = m_str; *s != 0; s++)
	{
		*s = ::tolower(*s);
	}
	return *this;
}


//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::MakeUpper()
{
	_MakeUnique();
	for (value_type *s = m_str; *s != 0; s++)
	{
		*s = ::toupper(*s);
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::Trim()
{
	return TrimRight().TrimLeft();
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::Trim(value_type ch)
{
	_MakeUnique();
	const value_type chset[2] = { ch, 0 };
	return TrimRight(chset).TrimLeft(chset);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::Trim(const value_type *sCharSet)
{
	_MakeUnique();
	return TrimRight(sCharSet).TrimLeft(sCharSet);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::TrimRight(value_type ch)
{
	const value_type chset[2] = { ch, 0 };
	return TrimRight(chset);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::TrimRight(const value_type *sCharSet)
{
	if (!sCharSet || !(*sCharSet) || length() < 1)
		return *this;

	const value_type *last = m_str + length() - 1;
	const value_type *str = last;
	while ((str != m_str) && (_strchr(sCharSet, *str) != 0))
		str--;

	if (str != last)
	{
		// Just shrink length of the string.
		size_type nNewLength = (size_type)(str - m_str) + 1; // m_str can change in _MakeUnique
		_MakeUnique();
		_header()->nLength = nNewLength;
		m_str[nNewLength] = 0;
	}

	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::TrimRight()
{
	if (length() < 1)
		return *this;

	const value_type *last = m_str + length() - 1;
	const value_type *str = last;
	while ((str != m_str) && strUtil::IsWhitespace((unsigned char)*str))
		str--;

	if (str != last)		// something changed?
	{
		// Just shrink length of the string.
		size_type nNewLength = (size_type)(str - m_str) + 1; // m_str can change in _MakeUnique
		_MakeUnique();
		_header()->nLength = safe_static_cast<int, size_t>(nNewLength);
		m_str[nNewLength] = 0;
	}

	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::TrimLeft(value_type ch)
{
	const value_type chset[2] = { ch, 0 };
	return TrimLeft(chset);
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::TrimLeft(const value_type *sCharSet)
{
	if (!sCharSet || !(*sCharSet))
		return *this;

	const value_type *str = m_str;
	while ((*str != 0) && (_strchr(sCharSet, *str) != 0))
		str++;

	if (str != m_str)
	{
		size_type nOff = (size_type)(str - m_str); // m_str can change in _MakeUnique
		_MakeUnique();
		size_type nNewLength = length() - nOff;
		_move(m_str, m_str + nOff, nNewLength + 1);
		_header()->nLength = nNewLength;
		m_str[nNewLength] = 0;
	}

	return *this;
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T>& StrRef<T>::TrimLeft()
{
	const value_type *str = m_str;
	while ((*str != 0) && strUtil::IsWhitespace((unsigned char)*str))
		str++;

	if (str != m_str)
	{
		size_type nOff = (size_type)(str - m_str); // m_str can change in _MakeUnique
		_MakeUnique();
		size_type nNewLength = length() - nOff;
		_move(m_str, m_str + nOff, nNewLength + 1);
		_header()->nLength = safe_static_cast<int, size_t>(nNewLength);
		m_str[nNewLength] = 0;
	}

	return *this;
}

template <class T>
inline StrRef<T> StrRef<T>::Right(size_type count) const
{
	if (count == npos)
		return StrRef<T>();
	else if (count > length())
		return *this;

	return StrRef<T>(m_str + length() - count, count);
}

template <class T>
inline StrRef<T> StrRef<T>::Left(size_type count) const
{
	if (count == npos)
		return StrRef<T>();
	else if (count > length())
		count = length();

	return StrRef<T>(m_str, count);
}

// strspn equivalent
template <class T>
inline StrRef<T> StrRef<T>::SpanIncluding(const_str charSet) const
{
	return Left((size_type)strspn(m_str, charSet));
}

// strcspn equivalent
template <class T>
inline StrRef<T> StrRef<T>::SpanExcluding(const_str charSet) const
{
	return Left((size_type)strcspn(m_str, charSet));
}

//////////////////////////////////////////////////////////////////////////
template <class T>
inline StrRef<T> StrRef<T>::Tokenize(const_str charSet, int &nStart) const
{
	if (nStart < 0)
	{
		return StrRef<T>();
	}

	if (!charSet)
		return *this;

	const_str sPlace = m_str + nStart;
	const_str sEnd = m_str + length();
	if (sPlace < sEnd)
	{
		int nIncluding = (int)strspn(sPlace, charSet);

		if ((sPlace + nIncluding) < sEnd)
		{
			sPlace += nIncluding;
			int nExcluding = (int)strcspn(sPlace, charSet);
			int nFrom = nStart + nIncluding;
			nStart = nFrom + nExcluding + 1;

			return substr(nFrom, nExcluding);
		}
	}
	// Return empty string.
	nStart = -1;
	return StrRef<T>();
}



typedef StrRef<char> string;

*/