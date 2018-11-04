#include "EngineCommon.h"
#include "LoggerFileWritePolicy.h"

#include <IFileSys.h>


X_NAMESPACE_BEGIN(core)

xLoggerFileWritePolicy::xLoggerFileWritePolicy(IFileSys* pFileSystem, const Path<>& path) :
    pFileSystem_(X_ASSERT_NOT_NULL(pFileSystem)),
    pFile_(nullptr),
    path_(path)
{
}

void xLoggerFileWritePolicy::Init(void)
{
    auto mode = FileFlag::WRITE | FileFlag::RECREATE | FileFlag::WRITE_FLUSH;

    pFile_ = pFileSystem_->openFile(path_, mode);
}

void xLoggerFileWritePolicy::Exit(void)
{
    if (pFile_) {
        pFileSystem_->closeFile(pFile_);
        pFile_ = nullptr;
    }
}

void xLoggerFileWritePolicy::WriteLog(const LoggerBase::Line& line, uint32_t length)
{
    pFile_->write(line, length);
}

void xLoggerFileWritePolicy::WriteWarning(const LoggerBase::Line& line, uint32_t length)
{
    pFile_->write(line, length);
}

void xLoggerFileWritePolicy::WriteError(const LoggerBase::Line& line, uint32_t length)
{
    pFile_->write(line, length);
}

void xLoggerFileWritePolicy::WriteFatal(const LoggerBase::Line& line, uint32_t length)
{
    pFile_->write(line, length);
}

void xLoggerFileWritePolicy::WriteAssert(const LoggerBase::Line& line, uint32_t length)
{
    pFile_->write(line, length);
}

void xLoggerFileWritePolicy::WriteAssertVariable(const LoggerBase::Line& line, uint32_t length)
{
    pFile_->write(line, length);
}

X_NAMESPACE_END