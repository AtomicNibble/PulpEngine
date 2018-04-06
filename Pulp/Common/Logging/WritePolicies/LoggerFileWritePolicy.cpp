#include "xLib.h"

#include "LoggerFileWritePolicy.h"

#include "FileSystem/File.h"

X_NAMESPACE_BEGIN(core)

xLoggerFileWritePolicy::xLoggerFileWritePolicy(FileSystem* fileSystem, const Pathname& path) :
    m_fileSystem(fileSystem),
    m_path(path)
{
}

void xLoggerFileWritePolicy::Init(void)
{
    m_file = m_fileSystem->Open(m_path.c_str(), FileSystem::Mode::WRITE | FileSystem::Mode::RECREATE | FileSystem::Mode::WRITE_FLUSH);
    m_file->WriteString("");
}

void xLoggerFileWritePolicy::Exit(void)
{
    m_fileSystem->Close(m_file);
}

void xLoggerFileWritePolicy::WriteLog(const LoggerBase::Line& line, uint32_t length)
{
    m_file->Write(line, length);
}

void xLoggerFileWritePolicy::WriteWarning(const LoggerBase::Line& line, uint32_t length)
{
    m_file->Write(line, length);
}

void xLoggerFileWritePolicy::WriteError(const LoggerBase::Line& line, uint32_t length)
{
    m_file->Write(line, length);
}

void xLoggerFileWritePolicy::WriteFatal(const LoggerBase::Line& line, uint32_t length)
{
    m_file->Write(line, length);
}

void xLoggerFileWritePolicy::WriteAssert(const LoggerBase::Line& line, uint32_t length)
{
    m_file->Write(line, length);
}

void xLoggerFileWritePolicy::WriteAssertVariable(const LoggerBase::Line& line, uint32_t length)
{
    m_file->Write(line, length);
}

X_NAMESPACE_END