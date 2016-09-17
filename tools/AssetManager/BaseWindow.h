#pragma once

#include <QObject>


class QStatusBar;

X_NAMESPACE_BEGIN(assman)

class BaseWindow : public  QWidget
{
	Q_OBJECT

public:
	static const int borderWidth = 4;
	static const int borderWidthGUI = 2;
	static const int borderHeightGUI = 2;

public:
	BaseWindow(QWidget* Parent = 0);
	~BaseWindow();

	void setWindowTitle(const QString& title);

	void setCentralWidget(QWidget* wiget);
	void setCentralWidget(QLayout* layout);
	void setStatusBar(QStatusBar* statusbar);
	void setMainLayoutName(const QString& name);


public slots:
	void raiseWindow(void);

signals:
	void WindowTitleChanged(void);
	void windowActivated(void);

private:

	QGridLayout	mainLayout_;
	QWidget*	centralWidget_;
};


X_NAMESPACE_END