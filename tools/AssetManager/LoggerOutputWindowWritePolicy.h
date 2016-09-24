#pragma once

#include <Logging\LoggerBase.h>
#include <QTextCharFormat>

X_NAMESPACE_BEGIN(assman)

class OutputWindow;


class LoggerOutputWindowWritePolicy
{
public:
	explicit LoggerOutputWindowWritePolicy(OutputWindow* pOutWindow);

	void Init(void);
	void Exit(void);

	void WriteLog(const core::LoggerBase::Line& line, uint32_t length);
	void WriteWarning(const core::LoggerBase::Line& line, uint32_t length);
	void WriteError(const core::LoggerBase::Line& line, uint32_t length);
	void WriteFatal(const core::LoggerBase::Line& line, uint32_t length);
	void WriteAssert(const core::LoggerBase::Line& line, uint32_t length);
	void WriteAssertVariable(const core::LoggerBase::Line& line, uint32_t length);

private:
	OutputWindow* pOutWindow_;
	core::CriticalSection cs_;

	QTextCharFormat warningFmt_;
	QTextCharFormat errorFmt_;
};


X_NAMESPACE_END