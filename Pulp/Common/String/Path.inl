

Path::Path()
{

}

Path::Path(const Path& oth)
{
	append(oth.c_str(), (uint32_t)oth.length());
}

Path::Path(const char* const str) : StackString<MAX_PATH>(str)
{

}

const char* Path::fileName(void) const
{
	const char* res = findLast(NATIVE_SLASH);

	if (!res || res == end())
	{
		// check both :Z
		res = findLast(NON_NATIVE_SLASH);
		if (!res || res == end())
			return str_;
	}
	return res + 1;
}

const char* Path::extension(void) const
{
	const char* res = findLast('.');

	if (!res)
		return str_;
	return res;
}

void Path::setExtension(const char* extension)
{
	const char* remove = findLast('.');	// need to remvoe a extension?
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


void Path::setFileName(const char* filename)
{
	// place in temp buffer otherwise
	// the original file name would be update
	// while we are replacing.

	const char* name = fileName();

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

void Path::operator=(const char* str)
{
	len_ = strUtil::strlen(str);
	len_ = core::Min<uint32_t>(len_, MAX_PATH);
	memcpy(str_, str, len_+1);
}

// -----------------------------------------------

const Path Path::operator/(const Path& oth) const
{
	Path path(*this);
	path.append(oth.c_str());
	return path;
}

const Path Path::operator/(const char* str) const
{
	Path path(*this);
	path.append(str);
	return path;
}

const Path& Path::operator/=(const Path& oth)
{
	append(oth.c_str(), (uint32_t)oth.length());
	return *this;
}

const Path& Path::operator/=(const char* str)
{
	append(str);
	return *this;
}


// -----------------------------------------------

inline void Path::ensureSlash(void)
{
	if (this->len_ > 0) {
		stripTrailing(NATIVE_SLASH);
		append(NATIVE_SLASH, 1);
	}
}

inline void Path::replaceSeprators(void)
{
	replaceAll(NON_NATIVE_SLASH, NATIVE_SLASH);
}

inline void Path::removeFileName(void)
{
	this->replace(this->fileName(),"");
}

inline void Path::removeExtension(void)
{
	setExtension("");
}

inline void Path::removeTrailingSlash(void)
{
	replaceSeprators();
	stripTrailing(NATIVE_SLASH);
}

inline bool Path::isAbsolute(void) const
{
	return	str_[0] == NATIVE_SLASH ||
		str_[0] == NON_NATIVE_SLASH ||
		str_[1] == ':';
}

