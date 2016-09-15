#pragma once

#include <QObject>

class QEvent;

X_NAMESPACE_BEGIN(assman)

class VersionDialog : public QDialog
{
	Q_OBJECT
public:
	explicit VersionDialog(QWidget *parent);

	bool event(QEvent *event);

};

X_NAMESPACE_END