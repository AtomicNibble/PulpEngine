#include "logging.h"


Q_LOGGING_CATEGORY(logCatAssetDb, "assetDb")
Q_LOGGING_CATEGORY(logCatAssetDbView, "assetDb.view")


bool debugLogging = true;



void redirectQtMsgToEngineLog(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	QByteArray localMsg = msg.toLocal8Bit();

	X_NAMESPACE(core)::SourceInfo srcInfo("core", context.file, context.line, context.function, context.function);

 	switch (type)
	{
	case QtDebugMsg:
		gEnv->pLog->Log(srcInfo, "Qt", 0, localMsg.constData());
		break;
	case QtInfoMsg:
		gEnv->pLog->Log(srcInfo, "Qt", 0, localMsg.constData());
		break;
	case QtWarningMsg:
		gEnv->pLog->Warning(srcInfo, "Qt", localMsg.constData());
		break;
	case QtCriticalMsg:
		gEnv->pLog->Error(srcInfo, "Qt", localMsg.constData());
		break;
	case QtFatalMsg:
		gEnv->pLog->Fatal(srcInfo, "Qt", localMsg.constData());

	}
}
