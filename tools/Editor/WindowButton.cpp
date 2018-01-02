#include "stdafx.h"
#include "WindowButton.h"

X_NAMESPACE_BEGIN(editor)


WindowButton::WindowButton(ButtonType type, QWidget *parent) : QPushButton(parent),
type_(type),
state_(STATE_NORMAL),
pNormal_(nullptr),
pHovered_(nullptr),
pClicked_(nullptr)
{
}

WindowButton::~WindowButton()
{
	delete pNormal_;
	delete pHovered_;
	delete pClicked_;
}



void WindowButton::resizeEvent(QResizeEvent *event)
{
	Q_UNUSED(event);

	InitPixmaps(event->size());
}

void WindowButton::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

	QPainter painter(this);

	if (isEnabled())
	{
		switch (state_)
		{
		case STATE_NORMAL:
			if (pNormal_ != nullptr) painter.drawPixmap(0, 0, *pNormal_);
			break;
		case STATE_HOVERED:
			if (pHovered_ != nullptr) painter.drawPixmap(0, 0, *pHovered_);
			break;
		case STATE_CLICKED:
			if (pClicked_ != nullptr) painter.drawPixmap(0, 0, *pClicked_);
			break;
		}
	}
	else
	{
		if (pNormal_ != nullptr) {
			painter.drawPixmap(0, 0, *pNormal_);
		}
	}
}

void WindowButton::enterEvent(QEvent *event)
{
	Q_UNUSED(event);

	state_ = STATE_HOVERED;

	update();
}

void WindowButton::leaveEvent(QEvent *event)
{
	Q_UNUSED(event);

	state_ = STATE_NORMAL;

	update();
}

void WindowButton::mousePressEvent(QMouseEvent *event)
{
	QAbstractButton::mousePressEvent(event);

	state_ = STATE_CLICKED;

	update();
}

void WindowButton::mouseReleaseEvent(QMouseEvent *event)
{
	QAbstractButton::mouseReleaseEvent(event);

	if (underMouse())
		state_ = STATE_HOVERED;
	else            
		state_ = STATE_NORMAL;

	update();
}


void WindowButton::InitPixmaps(const QSize& size)
{
	// Delete previous button
	InitPixmap(&pNormal_);
	InitPixmap(&pHovered_);
	InitPixmap(&pClicked_);

	switch (type_)
	{
	case BUTTON_MINIMIZE:
		InitMinimize(size);
		break;
	case BUTTON_MAXIMIZE:
		InitMaximize(size);
		break;
	case BUTTON_CLOSE:
		InitClose(size);
		break;
	}
}

void WindowButton::InitPixmap(QPixmap **pixmap)
{
	delete *pixmap;

	*pixmap = new QPixmap(size());

	(*pixmap)->fill(Qt::transparent);
}


void SetBackCol(QPainter& painter, QColor col)
{
	painter.setPen(QPen(col));
	painter.setBrush(QBrush(col));
}

void WindowButton::InitMinimize(const QSize& size)
{
	QRect rect(0, 0, size.width() - 1, size.height() - 1);

	int bwidth = 10;
	int bheight = 3;
	int x_start = (rect.width() - bwidth) / 2; // center it
	int y_start = 16;


	/********** Button's symbol **********/
	QPolygon symbol;

	symbol << QPoint(x_start, y_start) // TL
		<< QPoint(x_start + bwidth, y_start) // TR
		<< QPoint(x_start + bwidth, y_start + bheight)   // BR
		<< QPoint(x_start, y_start + bheight);  // BL
												/*************************************/

	QPainter painter;

	QBrush shape_col(0xd0d0d0);


	/********** Normal **********/
	painter.begin(pNormal_);

	painter.setPen(Qt::NoPen);
	painter.setBrush(shape_col);
	painter.drawPolygon(symbol);

	painter.end();
	/****************************/

	/********** Hovered **********/
	painter.begin(pHovered_);

	SetBackCol(painter, 0x404040);
	painter.drawRect(rect);

	painter.setPen(Qt::NoPen);
	painter.setBrush(shape_col);
	painter.drawPolygon(symbol);

	painter.end();
	/*****************************/

	/********** Clicked **********/
	painter.begin(pClicked_);

	SetBackCol(painter, 0x3281FF);
	painter.drawRect(rect);

	painter.setPen(Qt::NoPen);
	painter.setBrush(shape_col);
	painter.drawPolygon(symbol);

	painter.end();
	/*****************************/
}

void WindowButton::InitMaximize(const QSize& size)
{
	QRect rect(0, 0, size.width() - 1, size.height() - 1);

	int bwidth = 8;
	int bheight = 6;
	int bCapsize = 3;
	int x_start = (rect.width() - bwidth) / 2; // center it
	int y_start = 10;
	int y_istart = y_start + bCapsize;

	/********** Button's symbol **********/
	QPolygon symbol1, symbol2;

	symbol1 << QPoint(x_start, y_start)                       // top left
		<< QPoint(x_start + bwidth, y_start)              // top right
		<< QPoint(x_start + bwidth, y_start + bCapsize)   // bottom right
		<< QPoint(x_start, y_start + bCapsize);           // bottom left

	symbol2 << QPoint(x_start, y_istart)                      // top left
		<< QPoint(x_start + bwidth, y_istart)             // top right
		<< QPoint(x_start + bwidth, y_istart + bheight)   // bottom right
		<< QPoint(x_start, y_istart + bheight);           // bottom left
														  /*************************************/

	QPainter painter;

	QColor shape_col(0xd0d0d0);

	/********** Normal **********/
	painter.begin(pNormal_);

	painter.setPen(QPen(shape_col));
	painter.setBrush(QBrush(shape_col));

	painter.drawPolygon(symbol1);
	painter.setBrush(Qt::NoBrush);
	painter.drawPolygon(symbol2);

	painter.end();
	/****************************/

	/********** Hovered **********/
	painter.begin(pHovered_);

	SetBackCol(painter, 0x404040);
	painter.drawRect(rect);

	painter.setPen(QPen(shape_col));
	painter.setBrush(QBrush(shape_col));

	painter.drawPolygon(symbol1);
	painter.setBrush(Qt::NoBrush);
	painter.drawPolygon(symbol2);


	painter.end();
	/*****************************/

	/********** Clicked **********/
	painter.begin(pClicked_);

	SetBackCol(painter, 0x3281FF);
	painter.drawRect(rect);

	painter.setPen(QPen(shape_col));
	painter.setBrush(QBrush(shape_col));

	painter.drawPolygon(symbol1);
	painter.setBrush(Qt::NoBrush);
	painter.drawPolygon(symbol2);

	painter.end();
	/*****************************/
}


void WindowButton::InitClose(const QSize& size)
{
	QRect rect(0, 0, size.width() - 1, size.height() - 1);

	int x_size = 8;
	int x_start = (rect.width() - x_size) / 2; // center it
	int y_start = 10;

	/********** Button's symbol **********/
	QLine symbol1(
		QPoint(x_start, y_start), // top left
		QPoint(x_start + x_size, y_start + x_size) // bottom right
	);
	QLine symbol2(
		QPoint(x_start + x_size, y_start), // top right
		QPoint(x_start, y_start + x_size) // bottom left
	);


	/*************************************/
	QPainter painter;

	QBrush shape_col(0xd0d0d0);

	/********** Normal **********/
	painter.begin(pNormal_);

	painter.setPen(QPen(shape_col, 2.0));

	painter.drawLine(symbol1);
	painter.drawLine(symbol2);

	painter.end();
	/****************************/


	/********** Hovered **********/
	painter.begin(pHovered_);

	SetBackCol(painter, 0x404040);
	painter.drawRect(rect);

	painter.setPen(QPen(shape_col, 2.0));

	painter.drawLine(symbol1);
	painter.drawLine(symbol2);

	painter.end();
	/*****************************/

	/********** Clicked **********/
	painter.begin(pClicked_);

	SetBackCol(painter, 0x3281FF);
	painter.drawRect(rect);

	painter.setPen(QPen(shape_col, 2.0));

	painter.drawLine(symbol1);
	painter.drawLine(symbol2);

	painter.end();
	/*****************************/
}


X_NAMESPACE_END