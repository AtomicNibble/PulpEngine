#include "LoggerOutputWindowWritePolicy.h"
#include "OutputWindowWidget.h"

X_NAMESPACE_BEGIN(assman)


LoggerOutputWindowWritePolicy::LogData::LogData(const QLatin1String& latin, const QTextCharFormat& fmt) :
	msg_(latin),
	fmt_(fmt)
{

}

// -------------------------------------------------------

LoggerOutputWindowWritePolicy::LoggerOutputWindowWritePolicy() :
	pOutWindow_(nullptr),
	cs_(10),
	logVec_(g_arena)
{
}

void LoggerOutputWindowWritePolicy::Init()
{
	warningFmt_.setForeground(QBrush(QColor("yellow")));
	errorFmt_.setForeground(QBrush(QColor("red")));

	X_ASSERT(msgFmt_.isValid(), "Not valid")();
	X_ASSERT(warningFmt_.isValid(), "Not valid")();
	X_ASSERT(errorFmt_.isValid(), "Not valid")();

	uiThreadId_ = core::Thread::GetCurrentID();
}

void LoggerOutputWindowWritePolicy::setOutputWindow(OutputWindow* pOutWindow)
{
	pOutWindow_ = pOutWindow;

	connect(this, &LoggerOutputWindowWritePolicy::msgsAdded, this, &LoggerOutputWindowWritePolicy::submitMsgs, 
		Qt::ConnectionType::QueuedConnection);
}

void LoggerOutputWindowWritePolicy::Exit(void)
{
	pOutWindow_ = nullptr; // meh
}

void LoggerOutputWindowWritePolicy::WriteLog(const core::LoggerBase::Line& line, uint32_t length)
{
	WriteInternal(line, length, msgFmt_);
}

void LoggerOutputWindowWritePolicy::WriteWarning(const core::LoggerBase::Line& line, uint32_t length)
{
	WriteInternal(line, length, warningFmt_);
}

void LoggerOutputWindowWritePolicy::WriteError(const core::LoggerBase::Line& line, uint32_t length)
{
	WriteInternal(line, length, errorFmt_);
}

void LoggerOutputWindowWritePolicy::WriteFatal(const core::LoggerBase::Line& line, uint32_t length)
{
	WriteInternal(line, length, errorFmt_);
}

void LoggerOutputWindowWritePolicy::WriteAssert(const core::LoggerBase::Line& line, uint32_t length)
{
	WriteInternal(line, length, errorFmt_);
}

void LoggerOutputWindowWritePolicy::WriteAssertVariable(const core::LoggerBase::Line& line, uint32_t length)
{
	WriteInternal(line, length, errorFmt_);
}

void LoggerOutputWindowWritePolicy::WriteInternal(const core::LoggerBase::Line& line, uint32_t length, const QTextCharFormat& fmt)
{
	QLatin1String tmpLatin(line, length);

	core::CriticalSection::ScopedLock lock(cs_);

	logVec_.emplace_back(tmpLatin, fmt);

	if (logVec_.size() == 1) {
		emit msgsAdded();
	}
}

void LoggerOutputWindowWritePolicy::submitMsgs(void)
{
	X_ASSERT(core::Thread::GetCurrentID() == uiThreadId_, "Can only submit msg's from the ui thread")();

	// could swap vec to reduce contention.
	core::CriticalSection::ScopedLock lock(cs_);

	bool atBottom;
	pOutWindow_->appendBlockBegin(atBottom);

	for (const auto& msg : logVec_) {
		pOutWindow_->appendMessage(msg.msg_, msg.fmt_);
	}

	pOutWindow_->appendBlockEnd(atBottom);

	logVec_.clear();
}

X_NAMESPACE_END