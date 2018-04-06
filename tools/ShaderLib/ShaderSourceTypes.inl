
X_NAMESPACE_BEGIN(render)

namespace shader
{
    X_INLINE const core::string& SourceFile::getName(void) const
    {
        return name_;
    }

    X_INLINE const SourceFile::ByteArr& SourceFile::getFileData(void) const
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

    X_INLINE void SourceFile::setFileData(const ByteArr& data, uint32_t crc)
    {
        fileData_ = data;
        sourceCrc32_ = crc;
    }

    X_INLINE void SourceFile::setFileData(ByteArr&& data, uint32_t crc)
    {
        fileData_ = std::move(data);
        sourceCrc32_ = crc;
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
#if X_ENABLE_RENDER_SHADER_RELOAD
        LockType::ScopedLock writeLock(lock);

        if (refrences_.find(name) == RefrenceArr::invalid_index) {
            refrences_.push_back(name);
        }
#else
        X_UNUSED(name);
#endif // !X_ENABLE_RENDER_SHADER_RELOAD
    }

    X_INLINE void SourceFile::removeRefrence(const core::string& name)
    {
#if X_ENABLE_RENDER_SHADER_RELOAD
        LockType::ScopedLock writeLock(lock);

        refrences_.remove(name);
#else
        X_UNUSED(name);
#endif // !X_ENABLE_RENDER_SHADER_RELOAD
    }

} // namespace shader

X_NAMESPACE_END