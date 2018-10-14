#pragma once

#ifndef X_CORE_FILESYS_DISK_FILE_H_
#define X_CORE_FILESYS_DISK_FILE_H_

#include <IFileSys.h>

#include X_INCLUDE(X_PLATFORM/OsFile.h)

X_NAMESPACE_BEGIN(core)

class XDiskFile : public XFile
{
public:
    XDiskFile(const core::Path<wchar_t>& path, IFileSys::FileFlags mode);
    ~XDiskFile();

    inline bool valid(void) const {
        return file_.valid();
    }

    virtual size_t read(void* pBuffer, size_t length) X_OVERRIDE;
    virtual size_t write(const void* pBuffer, size_t length) X_OVERRIDE;
    virtual void seek(int64_t position, SeekMode::Enum origin) X_OVERRIDE;
    virtual uint64_t remainingBytes(void) const X_OVERRIDE;
    virtual uint64_t tell(void) const X_OVERRIDE;
    virtual void setSize(int64_t numBytes) X_OVERRIDE;

private:
    OsFile file_;
};

X_NAMESPACE_END

#endif // !X_CORE_FILESYS_DISK_FILE_H_