#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(editor)


class SplitterOrView;
class CustomTabWidget;

class CustomTabWidgetBar : public QTabBar
{
	Q_OBJECT
public:
	CustomTabWidgetBar(CustomTabWidget* parent);
	~CustomTabWidgetBar(void);

protected:
	void mousePressEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);

signals:


private:
	CustomTabWidget*  tabwidget_;
	QPoint            dragStartPos_;
	QPoint            tabOffset_;
	int32_t           tabIdx_;
	bool              inMove_;
};


class CustomTabWidget : public QTabWidget
{
	Q_OBJECT

public:
	friend class CustomTabWidgetBar;

public:
	CustomTabWidget(QWidget * parent = 0);

private:
	void detachTab(int32_t index, QPoint pos);

protected:
	CustomTabWidgetBar* pTabBar_;
};




X_NAMESPACE_END