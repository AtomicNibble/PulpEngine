
X_NAMESPACE_BEGIN(render)

namespace shader
{


	X_INLINE const core::string& SourceFile::getName(void) const
	{
		return name_;
	}

	X_INLINE const core::string& SourceFile::getFileName(void) const
	{
		return fileName_;
	}

	X_INLINE const core::string& SourceFile::getFileData(void) const
	{
		return fileData_;
	}

	X_INLINE uint32_t SourceFile::getSourceCrc32(void) const
	{
		return sourceCrc32_;
	}

	X_INLINE const SourceFile::IncludedSourceArr& SourceFile::getIncludeArr(void) const
	{
		return includedFiles_;
	}

	X_INLINE SourceFile::IncludedSourceArr& SourceFile::getIncludeArr(void)
	{
		return includedFiles_;
	}

	X_INLINE ILFlags SourceFile::getILFlags(void) const
	{
		return ILFlags_;
	}


	X_INLINE void SourceFile::setName(const core::string& name)
	{
		name_ = name;
	}

	X_INLINE void SourceFile::setFileName(const core::string& name)
	{
		fileName_ = name;
	}

	X_INLINE void SourceFile::setFileData(const core::string& name)
	{
		fileData_ = name;
	}

	X_INLINE void SourceFile::setSourceCrc32(uint32_t crc)
	{
		sourceCrc32_ = crc;
	}

	X_INLINE void SourceFile::setILFlags(ILFlags flags)
	{
		ILFlags_ = flags;
	}

	X_INLINE void SourceFile::addRefrence(const core::string& name)
	{
		refrences_.insert(name);
	}



} // namespace shader

X_NAMESPACE_END