#pragma once

#include <QObject>
#include <QStatusBar>


X_NAMESPACE_BEGIN(assman)

class QLabel;

class MyStatusBar : public QStatusBar
{
public:
	MyStatusBar();
	MyStatusBar(QWidget* parent);


private:
	void Create();

protected:
};



X_NAMESPACE_END