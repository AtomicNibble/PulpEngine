#pragma once

#include <QObject>
#include <QStatusBar>


X_NAMESPACE_BEGIN(editor)

class QLabel;

class MyStatusBar : public QStatusBar
{
public:
	MyStatusBar();
	MyStatusBar(QWidget* parent);



public slots:
	void showBusyBar(bool show);

private:
	void Create();

protected:
	QProgressBar* pProgress_;
};



X_NAMESPACE_END