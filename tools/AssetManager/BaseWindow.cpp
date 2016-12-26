#include "stdafx.h"
#include "BaseWindow.h"

#include "EditorView.h"
#include "EditorManager.h"

#include "WindowTileBar.h"

#include <QStatusBar>


X_NAMESPACE_BEGIN(assman)

BaseWindow::NcCursorPosCalculator::NcCursorPosCalculator()
{
	reset();
}

void BaseWindow::NcCursorPosCalculator::reset()
{
	onEdges = false;
	onLeftEdge = false;
	onRightEdge = false;
	onTopEdge = false;
	onBottomEdge = false;
	onTopLeftEdge = false;
	onBottomLeftEdge = false;
	onTopRightEdge = false;
	onBottomRightEdge = false;
}



void BaseWindow::NcCursorPosCalculator::recalculate(const QPoint& globalMousePos, const QRect& frameRect)
{
	int32_t globalMouseX = globalMousePos.x();
	int32_t globalMouseY = globalMousePos.y();

	int32_t frameX = frameRect.x();
	int32_t frameY = frameRect.y();

	int32_t frameWidth = frameRect.width();
	int32_t frameHeight = frameRect.height();

	onLeftEdge = globalMouseX >= frameX &&
		globalMouseX <= frameX + borderWidth;

	onRightEdge = globalMouseX >= frameX + frameWidth - borderWidth &&
		globalMouseX <= frameX + frameWidth;

	onTopEdge = globalMouseY >= frameY &&
		globalMouseY <= frameY + borderWidth;

	onBottomEdge = globalMouseY >= frameY + frameHeight - borderWidth &&
		globalMouseY <= frameY + frameHeight;

	onTopLeftEdge = onTopEdge && onLeftEdge;
	onBottomLeftEdge = onBottomEdge && onLeftEdge;
	onTopRightEdge = onTopEdge && onRightEdge;
	onBottomRightEdge = onBottomEdge && onRightEdge;

	//only these checks would be enough
	onEdges = onLeftEdge || onRightEdge ||
		onTopEdge || onBottomEdge;

}


// -----------------------------------------------------

BaseWindow::BaseWindow(QWidget * parent) :
	QWidget(parent),
	leftButtonPressed_(false),
	cursorShapeChanged_(false),
	customFrame_(true),
	mainLayout_(this),
	centralWidget_(nullptr),
	pCustomTitleBar_(nullptr)
{
	setMinimumSize(196, 96);

	if (customFrame_) {
		pCustomTitleBar_ = new WindowTitleBar();
		setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
	}

	setObjectName("BaseWindow");

	mainLayout_.setMargin(0);   //  No  space  between  window's  element  and  the  border
	mainLayout_.setSpacing(0);  //  No  space  between  window's  element
	setLayout(&mainLayout_);

	if (pCustomTitleBar_) {
		mainLayout_.addWidget(pCustomTitleBar_, 0, 0, 1, 3);

		connect(this, SIGNAL(WindowTitleChanged()),
			pCustomTitleBar_, SLOT(UpdateWindowTitle()));
		connect(this, SIGNAL(windowTitleChanged(QString)),
			pCustomTitleBar_, SLOT(UpdateWindowTitle()));

		pCustomTitleBar_->installEventFilter(this);
	}

	// left/right spacer
	mainLayout_.addItem(new QSpacerItem(borderWidthGUI, borderWidthGUI), 1, 0, 1, 1);
	mainLayout_.addItem(new QSpacerItem(borderWidthGUI, borderWidthGUI), 1, 2, 1, 1);
	// bottom space
	mainLayout_.addItem(new QSpacerItem(borderHeightGUI, borderHeightGUI), 2, 0, 1, 3);

	mainLayout_.setRowStretch(1, 1); //  Put  the  title  bar  at  the  top  of  the  window


	installEventFilter(this);

}


BaseWindow::~BaseWindow()
{
	if (pCustomTitleBar_) {
		delete pCustomTitleBar_;
	}
}


void BaseWindow::setWindowTitle(const QString& title)
{
	QWidget::setWindowTitle(title);

	emit WindowTitleChanged();
}


void BaseWindow::setCentralWidget(QWidget* widget)
{
	mainLayout_.addWidget(widget, 1, 1, 1, 1);
	centralWidget_ = widget;
}

void BaseWindow::setCentralWidget(QLayout* layout)
{
	mainLayout_.addLayout(layout, 1, 1, 1, 1);
}

void BaseWindow::setStatusBar(QStatusBar* statusbar)
{
	mainLayout_.addWidget(statusbar, 3, 0, 1, 3);
}

void BaseWindow::setMainLayoutName(const QString& name)
{
	mainLayout_.setObjectName(name);
}



void BaseWindow::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	if (e->type() == QEvent::ActivationChange)
	{
		bool active = isActiveWindow();

		if (pCustomTitleBar_) {
			pCustomTitleBar_->Focus(active);
		}

		if (isActiveWindow()) {
			emit windowActivated();
		}
	}
}


bool BaseWindow::eventFilter(QObject *o, QEvent *event)
{
	Q_UNUSED(o);

	switch (event->type())
	{
	case QEvent::MouseButtonPress:
		handleMousePressEvent(static_cast<QMouseEvent*>(event));
		break;
	case QEvent::MouseButtonRelease:
		handleMouseReleaseEvent(static_cast<QMouseEvent*>(event));
		break;
	case QEvent::MouseMove:
		handleMouseMoveEvent(static_cast<QMouseEvent*>(event));
		break;
	case QEvent::Leave:
		handleLeaveEvent(event);
		break;
	case QEvent::HoverMove:
		handleHoverMoveEvent(static_cast<QHoverEvent*>(event));
		break;
	}

	return false;
}

void BaseWindow::handleMousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		leftButtonPressed_ = true;

		QRect frameRect = frameGeometry();
		mousePos_.recalculate(event->globalPos(), frameRect);

		dragPos_ = event->globalPos() - frameRect.topLeft();
	}
}

void BaseWindow::handleMouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		leftButtonPressed_ = false;
		mousePos_.reset();
	}

	if (centralWidget_)
	{
		SplitterOrView *Splitter = qobject_cast<SplitterOrView *>(centralWidget_);
		if (Splitter) {
			EditorManager::splitDragEndWindow(this, Splitter);
		}
	}
}

void BaseWindow::handleMouseMoveEvent(QMouseEvent* event)
{
	// we need to check if this window is not hte main view
	// and do a dock check baby!
	// rekt.
	// the central widget is a view if it's floating.

	if (leftButtonPressed_)
	{
		if (!mousePos_.onEdges)
		{
			if (centralWidget_) {
				SplitterOrView* pSplitter = qobject_cast<SplitterOrView*>(centralWidget_);
				if (pSplitter) {
					EditorManager::floatDockCheck(pSplitter, event->globalPos());
				}
			}
		}
		else // if (mousePos_.onEdges)
		{
			resizeWidget(event->globalPos());
		}
	}
	else
	{
		updateCursorShape(event->globalPos());
	}
}


void BaseWindow::handleLeaveEvent(QEvent* /*event*/)
{
	if (!leftButtonPressed_) {
		unsetCursor();
	}
}

void BaseWindow::handleHoverMoveEvent(QHoverEvent* event)
{
	if (!leftButtonPressed_) {
		updateCursorShape(mapToGlobal(event->pos()));
	}
}


void BaseWindow::updateCursorShape(const QPoint& globalMousePos)
{
	if (isFullScreen() || isMaximized())
	{
		if (cursorShapeChanged_) {
			unsetCursor();
		}
		return;
	}

	moveMousePos_.recalculate(globalMousePos, frameGeometry());

	if (moveMousePos_.onTopLeftEdge || moveMousePos_.onBottomRightEdge)
	{
		setCursor(Qt::SizeFDiagCursor);
		cursorShapeChanged_ = true;
	}
	else if (moveMousePos_.onTopRightEdge || moveMousePos_.onBottomLeftEdge)
	{
		setCursor(Qt::SizeBDiagCursor);
		cursorShapeChanged_ = true;
	}
	else if (moveMousePos_.onLeftEdge || moveMousePos_.onRightEdge)
	{
		setCursor(Qt::SizeHorCursor);
		cursorShapeChanged_ = true;
	}
	else if (moveMousePos_.onTopEdge || moveMousePos_.onBottomEdge)
	{
		setCursor(Qt::SizeVerCursor);
		cursorShapeChanged_ = true;
	}

	else
	{
		if (cursorShapeChanged_)
		{
			unsetCursor();
			cursorShapeChanged_ = false;
		}
	}
}

void BaseWindow::resizeWidget(const QPoint& globalMousePos)
{
	QRect origRect = frameGeometry();

	int32_t left = origRect.left();
	int32_t top = origRect.top();
	int32_t right = origRect.right();
	int32_t bottom = origRect.bottom();
	origRect.getCoords(&left, &top, &right, &bottom);

	int32_t minWidth = minimumWidth();
	int32_t minHeight = minimumHeight();

	if (mousePos_.onTopLeftEdge)
	{
		left = globalMousePos.x();
		top = globalMousePos.y();
	}
	else if (mousePos_.onBottomLeftEdge)
	{
		left = globalMousePos.x();
		bottom = globalMousePos.y();
	}
	else if (mousePos_.onTopRightEdge)
	{
		right = globalMousePos.x();
		top = globalMousePos.y();
	}
	else if (mousePos_.onBottomRightEdge)
	{
		right = globalMousePos.x();
		bottom = globalMousePos.y();
	}
	else if (mousePos_.onLeftEdge)
	{
		left = globalMousePos.x();
	}
	else if (mousePos_.onRightEdge)
	{
		right = globalMousePos.x();
	}
	else if (mousePos_.onTopEdge)
	{
		top = globalMousePos.y();
	}
	else if (mousePos_.onBottomEdge)
	{
		bottom = globalMousePos.y();
	}

	QRect newRect(QPoint(left, top), QPoint(right, bottom));

	if (newRect.isValid())
	{
		if (minWidth > newRect.width())
		{
			//determine what has caused the width change.
			if (left != origRect.left()) {
				newRect.setLeft(origRect.left());
			}
			else {
				newRect.setRight(origRect.right());
			}
		}
		if (minHeight > newRect.height())
		{
			//determine what has caused the height change.
			if (top != origRect.top()) {
				newRect.setTop(origRect.top());
			} 
			else {
				newRect.setBottom(origRect.bottom());
			}
		}

		setGeometry(newRect);
	}
	else
	{
		//   qDebug() << "Calculated Rect is not valid" << newRect;
	}

}



void BaseWindow::raiseWindow(void)
{
	setWindowState(windowState() & ~Qt::WindowMinimized);

	raise();

	activateWindow();
}



X_NAMESPACE_END