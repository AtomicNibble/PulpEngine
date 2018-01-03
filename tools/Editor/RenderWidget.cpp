#include "stdafx.h"
#include "RenderWidget.h"

X_NAMESPACE_BEGIN(editor)


RenderWidget::RenderWidget(QWidget *parent) :
	QWidget(parent)
{
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAttribute(Qt::WA_PaintOnScreen, true);
}

RenderWidget::~RenderWidget()
{

}

QPaintEngine* RenderWidget::paintEngine(void) const
{
	return nullptr;
}

void RenderWidget::paintEvent(QPaintEvent* pEvent)
{
	X_UNUSED(pEvent);



}

X_NAMESPACE_END