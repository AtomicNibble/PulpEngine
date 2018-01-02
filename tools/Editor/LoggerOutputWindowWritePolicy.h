#pragma once

#include <qobject.h>

#include <Logging\LoggerBase.h>
#include <Containers\Array.h>
#include <QTextCharFormat>

X_NAMESPACE_BEGIN(editor)

class OutputWindow;


class LoggerOutputWindowWritePolicy : public QObject
{
	Q_OBJECT

	
	struct LogData
	{
		LogData(const QLatin1String& latin, const QTextCharFormat& fmt);

		QString msg_;
		const QTextCharFormat& fmt_;
	};

	typedef core::Array<LogData> logArr;

public:
	LoggerOutputWindowWritePolicy();

	void Init();
	void Exit(void);

	void setOutputWindow(OutputWindow* pOutWindow);

	void WriteLog(const core::LoggerBase::Line& line, uint32_t length);
	void WriteWarning(const core::LoggerBase::Line& line, uint32_t length);
	void WriteError(const core::LoggerBase::Line& line, uint32_t length);
	void WriteFatal(const core::LoggerBase::Line& line, uint32_t length);
	void WriteAssert(const core::LoggerBase::Line& line, uint32_t length);
	void WriteAssertVariable(const core::LoggerBase::Line& line, uint32_t length);

private:
	void WriteInternal(const core::LoggerBase::Line& line, uint32_t length, const QTextCharFormat& fmt);

signals:
	void msgsAdded(void);

private slots:
	void submitMsgs(void);

private:
	OutputWindow* pOutWindow_;
	core::CriticalSection cs_;

	uint32_t uiThreadId_;
	logArr logVec_;

	QTextCharFormat msgFmt_;
	QTextCharFormat warningFmt_;
	QTextCharFormat errorFmt_;
};


X_NAMESPACE_END