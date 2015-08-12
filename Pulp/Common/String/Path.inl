
template<typename TChar>
Path<TChar>::Path()
{

}

template<typename TChar>
Path<TChar>::Path(const Path& oth)
{
	append(oth.c_str(), (uint32_t)oth.length());
}

template<typename TChar>
Path<TChar>::Path(const TChar* const str) : StackString<MAX_PATH, TChar>(str)
{

}

template<typename TChar>
const TChar* Path<TChar>::fileName(void) const
{
	const TChar* res = findLast(NATIVE_SLASH);

	if (!res || res == end())
	{
		// check both :Z
		res = findLast(NON_NATIVE_SLASH);
		if (!res || res == end())
			return str_;
	}
	return res + 1;
}

template<typename TChar>
const TChar* Path<TChar>::extension(void) const
{
	const TChar* res = findLast('.');

	if (!res)
		return str_;
	return res;
}

template<typename TChar>
void Path<TChar>::setExtension(const TChar* extension)
{
	const TChar* remove = findLast('.');	// need to remvoe a extension?
	bool has_dot = (extension[0] == '.'); // new extension got a dot?
	bool is_blank = (extension[0] == '\0'); // 

	if (remove) { // replace making sure dot is kept.
		// check if the same.
		if (strUtil::IsEqual(has_dot ? remove : remove + 1, extension))
			return;

		// replace last.
		if (is_blank)
			--remove;
		replace(remove, ++remove, has_dot ? ++extension : extension);
		return;
	}

	if (!has_dot)
		append('.',1);
	
	append(extension);
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
		StackString<MAX_PATH> temp(str_, name); // want the text before filename
		temp.append(filename);

		*this = temp.c_str();
	}
}

template<typename TChar>
void Path<TChar>::operator=(const TChar* str)
{
	len_ = strUtil::strlen(str);
	len_ = core::Min<uint32_t>(len_, MAX_PATH);
	memcpy(str_, str, len_+1);
}

// -----------------------------------------------

template<typename TChar>
const Path<TChar> Path<TChar>::operator/(const Path<TChar>& oth) const
{
	Path path(*this);
	path.append(oth.c_str());
	return path;
}

template<typename TChar>
const Path<TChar> Path<TChar>::operator/(const TChar* str) const
{
	Path<TChar> path(*this);
	path.append(str);
	return path;
}

template<typename TChar>
const Path<TChar>& Path<TChar>::operator/=(const Path<TChar>& oth)
{
	append(oth.c_str(), (uint32_t)oth.length());
	return *this;
}

template<typename TChar>
const Path<TChar>& Path<TChar>::operator/=(const TChar* str)
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

template<typename TChar>
inline void Path<TChar>::replaceSeprators(void)
{
	replaceAll(NON_NATIVE_SLASH, NATIVE_SLASH);
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

template<typename TChar>
inline void Path<TChar>::removeExtension(void)
{
	setExtension("");
}

template<typename TChar>
inline void Path<TChar>::removeTrailingSlash(void)
{
	replaceSeprators();
	stripTrailing(NATIVE_SLASH);
}

template<typename TChar>
inline bool Path<TChar>::isAbsolute(void) const
{
	return	str_[0] == NATIVE_SLASH ||
		str_[0] == NON_NATIVE_SLASH ||
		str_[1] == ':';
}

