#include "stdafx.h"
#include "ColorPreviewWidget.h"


X_NAMESPACE_BEGIN(editor)


class ColorPreview::Private
{
public:
	Private() :
		col(Qt::red),
		back(Qt::darkGray, Qt::DiagCrossPattern),
		display_mode(DisplayMode::NoAlpha)
	{
	}

public:
	QColor col; ///< color to be viewed
	QColor comparison; ///< comparison color
	QBrush back;///< Background brush, visible on a transparent color
	DisplayMode display_mode; ///< How the color(s) are to be shown

};

ColorPreview::ColorPreview(QWidget* parent) :
	QWidget(parent), 
	p(new Private)
{
	p->back.setTexture(QPixmap(QLatin1String(":/misc/img/alphaback.png")));
}

ColorPreview::~ColorPreview()
{
	delete p;
}

void ColorPreview::setBackground(const QBrush& bk)
{
	p->back = bk;
	update();
}

QBrush ColorPreview::background(void) const
{
	return p->back;
}

ColorPreview::DisplayMode ColorPreview::displayMode(void) const
{
	return p->display_mode;
}

void ColorPreview::setDisplayMode(DisplayMode m)
{
	p->display_mode = m;
	update();
}

QColor ColorPreview::color(void) const
{
	return p->col;
}

QColor ColorPreview::comparisonColor(void) const
{
	return p->comparison;
}

QSize ColorPreview::sizeHint(void) const
{
	return QSize(24, 24);
}

void ColorPreview::paint(QPainter &painter, QRect rect) const
{
	QColor c1, c2;
	switch (p->display_mode) {
	case DisplayMode::NoAlpha:
		c1 = c2 = p->col.rgb();
		break;
	case DisplayMode::AllAlpha:
		c1 = c2 = p->col;
		break;
	case DisplayMode::SplitAlpha:
		c1 = p->col.rgb();
		c2 = p->col;
		break;
	case DisplayMode::SplitColor:
		c1 = p->comparison;
		c2 = p->col;
		break;
	}

	QStyleOptionFrame panel;
	panel.initFrom(this);
	panel.lineWidth = 2;
	panel.midLineWidth = 0;
	panel.state |= QStyle::State_Sunken;
	style()->drawPrimitive(QStyle::PE_Frame, &panel, &painter, this);
	const QRect r = style()->subElementRect(QStyle::SE_FrameContents, &panel, this);
	painter.setClipRect(r);

	if (c1.alpha() < 255 || c2.alpha() < 255) {
		painter.fillRect(0, 0, rect.width(), rect.height(), p->back);
	}

	const int32_t w = rect.width() / 2;
	const int32_t h = rect.height();
	painter.fillRect(0, 0, w, h, c1);
	painter.fillRect(w, 0, w, h, c2);
}

void ColorPreview::setColor(const QColor& c)
{
	p->col = c;
	update();
	emit colorChanged(c);
}

void ColorPreview::setComparisonColor(const QColor& c)
{
	p->comparison = c;
	update();
}

void ColorPreview::paintEvent(QPaintEvent* )
{
	QStylePainter painter(this);

	paint(painter, geometry());
}

void ColorPreview::resizeEvent(QResizeEvent* )
{
	update();
}

void ColorPreview::mouseReleaseEvent(QMouseEvent* ev)
{
	if (ev->button() != Qt::MouseButton::LeftButton) {
		return;
	}

	if (QRect(QPoint(0, 0), size()).contains(ev->pos())) {
		emit clicked();
	}
}

void ColorPreview::mouseMoveEvent(QMouseEvent* ev)
{
	if (ev->buttons() &Qt::LeftButton && !QRect(QPoint(0, 0), size()).contains(ev->pos()))
	{
		QMimeData* pData = new QMimeData;

		pData->setColorData(p->col);

		QPixmap preview(24, 24);
		preview.fill(p->col);

		QDrag* pDrag = new QDrag(this);
		pDrag->setMimeData(pData);
		pDrag->setPixmap(preview);
		pDrag->exec();
	}
}

X_NAMESPACE_END