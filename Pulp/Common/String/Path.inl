
template<typename TChar>
Path<TChar>::Path()
{

}

template<typename TChar>
Path<TChar>::Path(const Path& oth)
{
	append(oth.c_str(), oth.length());
}

template<>
template<>
inline Path<char>::Path(const Path<wchar_t>& oth)
{
	strUtil::Convert(oth.c_str(), str_, capacity());
	str_[oth.length()] = L'\0';
	len_ = oth.length();
}

template<>
template<>
inline Path<wchar_t>::Path(const Path<char>& oth)
{
	strUtil::Convert(oth.c_str(), str_, capacity());
	str_[oth.length()] = L'\0';
	len_ = oth.length();
}


template<typename TChar>
Path<TChar>::Path(const TChar* const str) : StackString<MAX_PATH, TChar>(str)
{

}

template<typename TChar>
Path<TChar>::Path(const TChar* const beginInclusive, const TChar* const endExclusive) : 
	StackString<MAX_PATH, TChar>(beginInclusive, endExclusive)
{

}


template<typename TChar>
const TChar* Path<TChar>::fileName(void) const
{
	const TChar* native = findLast(NATIVE_SLASH);
	// folder//
	if (native == end() - 1) {
		return native + 1;
	}

	const TChar* noneNative = findLast(NON_NATIVE_SLASH);
	// folder\ 
	if (noneNative == end() - 1) {
		return noneNative + 1;
	}

	if (!native && !noneNative)
		return str_;
	if (!noneNative && native)
		return native + 1;
	if (noneNative && !native)
		return noneNative + 1;


	// work out which is bigger
	const TChar* last = core::Max(native, noneNative);

	return last + 1;
}

template<typename TChar>
const TChar* Path<TChar>::extension(bool incDot) const
{
	const TChar* res = findLast('.');

	if (!res) {
		return str_;
	}

	if (incDot) {
		return res;
	}
	return res + 1;
}

template<typename TChar>
void Path<TChar>::setExtension(const TChar* extension)
{
	const TChar* remove = findLast('.');	// need to remvoe a extension?
	bool has_dot = (extension[0] == '.'); // new extension got a dot?
	bool is_blank = (extension[0] == '\0'); // 

	if (remove) { 
		trimRight(remove);
	}

	if (!is_blank) 
	{
		if (!has_dot) {
			append('.', 1);
		}

		append(extension);
	}
}

template<typename TChar>
void Path<TChar>::setFileName(const TChar* filename)
{
	// place in temp buffer otherwise
	// the original file name would be update
	// while we are replacing.

	const TChar* name = fileName();

	if (isEmpty() || (name == end()))
	{
		append(filename);
	}
	else 
	{
		StackString<MAX_PATH, TChar> temp(str_, name); // want the text before filename
		temp.append(filename);

		*this = temp.c_str();
	}
}

template<typename TChar>
void Path<TChar>::operator=(const TChar* str)
{
	len_ = strUtil::strlen(str);
	len_ = core::Min<size_t>(len_, MAX_PATH);
	memcpy(str_, str, (len_ + 1) * sizeof(TChar));
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
	append(oth.c_str(), oth.length());
	return *this;
}

template<typename TChar>
const Path<TChar>& Path<TChar>::operator/=(const TChar* str)
{
	ensureSlash();
	append(str);
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
	append(oth.c_str(), oth.length());
	return *this;
}

template<typename TChar>
const Path<TChar>& Path<TChar>::operator+=(const TChar* str)
{
	append(str);
	return *this;
}

// -----------------------------------------------

template<typename TChar>
inline void Path<TChar>::ensureSlash(void)
{
	if (this->len_ > 0) {
		stripTrailing(NATIVE_SLASH);
		append(NATIVE_SLASH, 1);
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
	stripTrailing(NATIVE_SLASH);
}

template<typename TChar>
inline size_t Path<TChar>::fillSpaceWithNullTerm(void)
{
	const size_t space = capacity() - length();

	std::memset(&str_[len_], '\0', space);

	return space;
}

template<typename TChar>
inline bool Path<TChar>::isAbsolute(void) const
{
	return	str_[0] == NATIVE_SLASH ||
		str_[0] == NON_NATIVE_SLASH ||
		str_[1] == ':';
}

template<typename TChar>
inline int8_t Path<TChar>::getDriveNumber(void) const
{
	if (length() > 1 && str_[1] == ':') {
		return safe_static_cast<int8_t,int32_t>(str_[0] - 'A');
	}

	return -1;
}