#include "logging.h"


Q_LOGGING_CATEGORY(logCatAssetDb, "assetDb")
Q_LOGGING_CATEGORY(logCatAssetDbView, "assetDb.view")


bool debugLogging = true;



void redirectQtMsgToEngineLog(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	QByteArray localMsg = msg.toLocal8Bit();

#if X_ENABLE_LOGGING_SOURCE_INFO
	X_NAMESPACE(core)::SourceInfo srcInfo("core", context.file, context.line, context.function, context.function);
#else
	X_UNUSED(context);
#endif // !X_ENABLE_LOGGING_SOURCE_INFO

 	switch (type)
	{
	case QtDebugMsg:
		gEnv->pLog->Log(X_SOURCE_INFO_LOG_CA(srcInfo) "Qt", 0, localMsg.constData());
		break;
	case QtInfoMsg:
		gEnv->pLog->Log(X_SOURCE_INFO_LOG_CA(srcInfo) "Qt", 0, localMsg.constData());
		break;
	case QtWarningMsg:
		gEnv->pLog->Warning(X_SOURCE_INFO_LOG_CA(srcInfo) "Qt", localMsg.constData());
		break;
	case QtCriticalMsg:
		gEnv->pLog->Error(X_SOURCE_INFO_LOG_CA(srcInfo) "Qt", localMsg.constData());
		break;
	case QtFatalMsg:
		gEnv->pLog->Fatal(X_SOURCE_INFO_LOG_CA(srcInfo) "Qt", localMsg.constData());

	}
}
