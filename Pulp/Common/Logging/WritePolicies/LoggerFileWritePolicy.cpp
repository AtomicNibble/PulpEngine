#include "EngineCommon.h"
#include "LoggerFileWritePolicy.h"

#include <IFileSys.h>


X_NAMESPACE_BEGIN(core)

LoggerFileWritePolicy::LoggerFileWritePolicy(IFileSys* pFileSystem, const Path<>& path) :
    pFileSystem_(X_ASSERT_NOT_NULL(pFileSystem)),
    pFile_(nullptr),
    path_(path)
{
}

void LoggerFileWritePolicy::Init(void)
{
    auto mode = FileFlag::WRITE | FileFlag::RECREATE | FileFlag::WRITE_FLUSH;

    pFile_ = pFileSystem_->openFile(path_, mode);
}

void LoggerFileWritePolicy::Exit(void)
{
    if (pFile_) {
        pFileSystem_->closeFile(pFile_);
        pFile_ = nullptr;
    }
}

void LoggerFileWritePolicy::WriteLog(const LoggerBase::Line& line, uint32_t length)
{
    pFile_->write(line, length);
}

void LoggerFileWritePolicy::WriteWarning(const LoggerBase::Line& line, uint32_t length)
{
    pFile_->write(line, length);
}

void LoggerFileWritePolicy::WriteError(const LoggerBase::Line& line, uint32_t length)
{
    pFile_->write(line, length);
}

void LoggerFileWritePolicy::WriteFatal(const LoggerBase::Line& line, uint32_t length)
{
    pFile_->write(line, length);
}

void LoggerFileWritePolicy::WriteAssert(const LoggerBase::Line& line, uint32_t length)
{
    pFile_->write(line, length);
}

void LoggerFileWritePolicy::WriteAssertVariable(const LoggerBase::Line& line, uint32_t length)
{
    pFile_->write(line, length);
}

X_NAMESPACE_END