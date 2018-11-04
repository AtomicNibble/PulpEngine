#pragma once

#ifndef X_LOGGERFILEWRITEPOLICY_H_
#define X_LOGGERFILEWRITEPOLICY_H_

#include "Logging\LoggerBase.h"

X_NAMESPACE_BEGIN(core)

struct IFileSys;
struct XFile;

class LoggerFileWritePolicy
{
public:
    LoggerFileWritePolicy(IFileSys* pFileSystem, const Path<>& path);

    void Init(void);
    void Exit(void);

    void WriteLog(const LoggerBase::Line& line, uint32_t length);
    void WriteWarning(const LoggerBase::Line& line, uint32_t length);
    void WriteError(const LoggerBase::Line& line, uint32_t length);
    void WriteFatal(const LoggerBase::Line& line, uint32_t length);
    void WriteAssert(const LoggerBase::Line& line, uint32_t length);
    void WriteAssertVariable(const LoggerBase::Line& line, uint32_t length);

private:
    IFileSys* pFileSystem_;
    XFile* pFile_;
    Path<> path_;
};

X_NAMESPACE_END

#endif // X_LOGGERFILEWRITEPOLICY_H_
