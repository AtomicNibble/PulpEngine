#pragma once

#include <QObject>


class QStatusBar;

X_NAMESPACE_BEGIN(editor)

class WindowTitleBar;

class BaseWindow : public  QWidget
{
	Q_OBJECT

	class NcCursorPosCalculator
	{

	public:
		NcCursorPosCalculator();

		void reset();
		void recalculate(const QPoint& globalMousePos, const QRect& frameRect);

	public:
		bool onEdges;
		bool onLeftEdge;
		bool onRightEdge;
		bool onTopEdge;
		bool onBottomEdge;
		bool onTopLeftEdge;
		bool onBottomLeftEdge;
		bool onTopRightEdge;
		bool onBottomRightEdge;
	};

public:
	static const int borderWidth = 4;
	static const int borderWidthGUI = 6;
	static const int borderHeightGUI = 6;

public:
	BaseWindow(QWidget* Parent = 0);
	~BaseWindow();

	void setWindowTitle(const QString& title);

	void setCentralWidget(QWidget* wiget);
	void setCentralWidget(QLayout* layout);
	void setStatusBar(QStatusBar* statusbar);
	void setMainLayoutName(const QString& name);

protected:
//	void showEvent(QShowEvent * event);

	void changeEvent(QEvent *e) X_OVERRIDE;
	bool eventFilter(QObject *o, QEvent *e) X_OVERRIDE;

	void handleMouseDblClickEvent(QMouseEvent* e);
	void handleMousePressEvent(QMouseEvent* event);
	void handleMouseReleaseEvent(QMouseEvent* event);
	void handleMouseMoveEvent(QMouseEvent* event);
	void handleLeaveEvent(QEvent* event);
	void handleHoverMoveEvent(QHoverEvent* event);

	void updateCursorShape(const QPoint& globalMousePos);
	void resizeWidget(const QPoint& globalMousePos);


public slots:
	void raiseWindow(void);

signals:
	void WindowTitleChanged(void);
	void windowActivated(void);

private:
	NcCursorPosCalculator  mousePos_;
	NcCursorPosCalculator  moveMousePos_;

	bool leftButtonPressed_;
	bool cursorShapeChanged_;
	bool customFrame_;

	QPoint      dragPos_;
	QPoint      diff_;
	QGridLayout	mainLayout_;
	QWidget*	centralWidget_;
	WindowTitleBar* pCustomTitleBar_;
};


X_NAMESPACE_END