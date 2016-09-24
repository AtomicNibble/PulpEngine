#include "stdafx.h"
#include "StyleUtilWidgets.h"

#include <QPainter>
#include <QStyleOption>
#include <QStylePainter>

X_NAMESPACE_BEGIN(assman)


StyledBar::StyledBar(QWidget *parent)
	: QWidget(parent)
{
	setObjectName("StyledBar");
	setProperty("panelwidget", true);
	setProperty("panelwidget_singlerow", true);
	setProperty("lightColored", false);

}

void StyledBar::setSingleRow(bool singleRow)
{
	setProperty("panelwidget_singlerow", singleRow);
}

bool StyledBar::isSingleRow(void) const
{
	return property("panelwidget_singlerow").toBool();
}

void StyledBar::setLightColored(bool lightColored)
{
	setProperty("lightColored", lightColored);
}

bool StyledBar::isLightColored(void) const
{
	return property("lightColored").toBool();
}

void StyledBar::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

#if 1
	QPainter painter(this);
	QStyleOption option;
	option.init(this);
	option.rect = rect();
	option.state = QStyle::State_Horizontal;
	style()->drawControl(QStyle::CE_ToolBar, &option, &painter, this);

#else
	QStyleOption opt;
	opt.init(this);
	QStylePainter p(this);
	p.drawPrimitive(QStyle::PE_Widget, opt);
#endif
}

// ----------------------------------------------------------------

StyledSeparator::StyledSeparator(QWidget *parent)
	: QWidget(parent)
{
	setFixedWidth(10);
}

void StyledSeparator::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)
	QPainter painter(this);
	QStyleOption option;
	option.rect = rect();
	option.state = QStyle::State_Horizontal;
	option.palette = palette();
	style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &option, &painter, this);
}


X_NAMESPACE_END