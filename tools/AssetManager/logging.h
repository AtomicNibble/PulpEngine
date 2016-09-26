#ifndef LOGGING_H
#define LOGGING_H


#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logCatAssetDb)
Q_DECLARE_LOGGING_CATEGORY(logCatAssetDbView)

extern bool debugLogging;

void redirectQtMsgToEngineLog(QtMsgType type, const QMessageLogContext &context, const QString &msg);


#endif // LOGGING_H
