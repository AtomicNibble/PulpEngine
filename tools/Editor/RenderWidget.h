#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(editor)


class RenderWidget : public QWidget
{
	Q_OBJECT

public:
	RenderWidget(QWidget *parent = 0);
	~RenderWidget();

protected:
	void paintEvent(QPaintEvent* pEvent);

private:
	QPaintEngine* paintEngine(void) const X_OVERRIDE;
};


X_NAMESPACE_END