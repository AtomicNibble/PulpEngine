#include "LoggerOutputWindowWritePolicy.h"
#include "OutputWindowWidget.h"

X_NAMESPACE_BEGIN(assman)


LoggerOutputWindowWritePolicy::LoggerOutputWindowWritePolicy(OutputWindow* pOutWindow) :
	pOutWindow_(pOutWindow),
	cs_(10)
{

}

void LoggerOutputWindowWritePolicy::Init(void)
{
	warningFmt_.setForeground(QBrush(QColor("yellow")));
	errorFmt_.setForeground(QBrush(QColor("red")));


}

void LoggerOutputWindowWritePolicy::Exit(void)
{
	pOutWindow_ = nullptr; // meh
}

void LoggerOutputWindowWritePolicy::WriteLog(const core::LoggerBase::Line& line, uint32_t length)
{
	// i don't think this will ever really get any contention,
	// but the engine expects all write polices to be safe to call from multiple threads.
	// so we make this writer conformant, using locks with low contention is cheap anyway.
	// this might actually need moving into the Output window...
	// if we allow logging calls not made by engine logger to go to output window.
	core::CriticalSection::ScopedLock lock(cs_); 

	pOutWindow_->appendMessage(line, length);
}

void LoggerOutputWindowWritePolicy::WriteWarning(const core::LoggerBase::Line& line, uint32_t length)
{
	core::CriticalSection::ScopedLock lock(cs_);

	pOutWindow_->appendMessage(line, length, warningFmt_);
}

void LoggerOutputWindowWritePolicy::WriteError(const core::LoggerBase::Line& line, uint32_t length)
{
	core::CriticalSection::ScopedLock lock(cs_);

	pOutWindow_->appendMessage(line, length, errorFmt_);
}

void LoggerOutputWindowWritePolicy::WriteFatal(const core::LoggerBase::Line& line, uint32_t length)
{
	core::CriticalSection::ScopedLock lock(cs_);

	pOutWindow_->appendMessage(line, length, errorFmt_);
}

void LoggerOutputWindowWritePolicy::WriteAssert(const core::LoggerBase::Line& line, uint32_t length)
{
	core::CriticalSection::ScopedLock lock(cs_);

	pOutWindow_->appendMessage(line, length, errorFmt_);
}

void LoggerOutputWindowWritePolicy::WriteAssertVariable(const core::LoggerBase::Line& line, uint32_t length)
{
	core::CriticalSection::ScopedLock lock(cs_);

	pOutWindow_->appendMessage(line, length, errorFmt_);
}



X_NAMESPACE_END