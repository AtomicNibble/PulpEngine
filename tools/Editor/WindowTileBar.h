#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(editor)


class WindowIcon;
class WindowButton;

class WindowTitleBar : public QWidget
{
	Q_OBJECT

	static const int32_t PIXELS_TO_ACT = 5;

public:
	explicit WindowTitleBar(QWidget *parent = 0);
	~WindowTitleBar();

	void Focus(bool f);

	public slots:
	void UpdateWindowTitle();

	void Minimized();
	void Maximized();
	void Quit();

protected:
	void resizeEvent(QResizeEvent  * event);
	void paintEvent(QPaintEvent   * event);

	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *e);

private:
	
	bool move_;
	bool _pad[3];

	QPoint diff_;
	WindowIcon* pIcon_;
	QLabel* pTitle_;
	WindowButton* pMinimize_;
	WindowButton* pMaximize_;
	WindowButton* pClose_;
};

X_NAMESPACE_END