#pragma once

#ifndef X_LOGGERFILEWRITEPOLICY_H_
#define X_LOGGERFILEWRITEPOLICY_H_

#include "FileSystem/Pathname.h"

X_NAMESPACE_BEGIN(core)

class FileSystem;
class File;

/// \ingroup Logging
/// \brief A class that implements a write policy for log messages.
/// \details This class implements the concepts of a write policy as expected by the Logger class. It writes all
/// log messages to a file.
/// \sa X_LOG0 X_LOG1 X_LOG2 X_WARNING X_ERROR X_FATAL Logger LoggerBase
class xLoggerFileWritePolicy
{
public:
    /// \brief Constructs a policy instance.
    /// \remark The policy stores a pointer to the file system internally, but does not take ownership. Users must ensure
    /// that the file system is not freed before the policy is.
    xLoggerFileWritePolicy(FileSystem* fileSystem, const Pathname& path);

    /// Creates a log file.
    void Init(void);

    /// Closes the log file.
    void Exit(void);

    /// Writes a log message to the file.
    void WriteLog(const LoggerBase::Line& line, uint32_t length);

    /// Writes a warning message to the file.
    void WriteWarning(const LoggerBase::Line& line, uint32_t length);

    /// Writes an error message to the file.
    void WriteError(const LoggerBase::Line& line, uint32_t length);

    /// Writes a fatal error message to the file.
    void WriteFatal(const LoggerBase::Line& line, uint32_t length);

    /// Writes an assert message to the file.
    void WriteAssert(const LoggerBase::Line& line, uint32_t length);

    /// Writes an assert variable message to the file.
    void WriteAssertVariable(const LoggerBase::Line& line, uint32_t length);

private:
    FileSystem* m_fileSystem;
    File* m_file;
    Pathname m_path;
};

X_NAMESPACE_END

#endif // X_LOGGERFILEWRITEPOLICY_H_
