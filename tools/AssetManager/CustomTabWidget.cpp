#include "stdafx.h"
#include "CustomTabWidget.h"

#include "EditorView.h"
#include "EditorManager.h"

X_NAMESPACE_BEGIN(assman)


namespace
{
	enum DragResult {
		DRAG_NONE,
		DRAG_MOVE_LEFT,
		DRAG_MOVE_RIGHT,
		DRAG_DETACH
	};


	QString DragResultToString(DragResult res)
	{
		switch (res)
		{
		case DRAG_NONE:
			return "None";
		case DRAG_MOVE_LEFT:
			return "Move:LEFT";
		case DRAG_MOVE_RIGHT:
			return "Move:RIGHT";
		case DRAG_DETACH:
			return "Detach";
		}
		return "Unknown";
	}


} // namespace


CustomTabWidgetBar::CustomTabWidgetBar(CustomTabWidget *widget) :
	QTabBar(widget),
	tabwidget_(widget),
	inMove_(false),
	tabIdx_(-1)
{
	setObjectName("assEntryBar");
	setAcceptDrops(true);

//	setElideMode(Qt::ElideRight);
	setSelectionBehaviorOnRemove(QTabBar::SelectLeftTab);
}

CustomTabWidgetBar::~CustomTabWidgetBar(void)
{

}


void CustomTabWidgetBar::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && count() > 0) 
	{
		QPoint& pos = event->pos();

		dragStartPos_ = pos;
		tabIdx_ = tabAt(pos);

		// the tab offset
		QRect rect = tabRect(tabIdx_);
		tabOffset_ = QPoint(pos.x() - rect.x(), pos.y());
	}

	QTabBar::mousePressEvent(event);
}

void CustomTabWidgetBar::mouseReleaseEvent(QMouseEvent* event)
{
	Q_UNUSED(event)

	inMove_ = false;

	QTabBar::mouseReleaseEvent(event);
}


void CustomTabWidgetBar::mouseMoveEvent(QMouseEvent* event)
{
	QPoint& pos = event->pos();

	if (tabIdx_ == -1) {
		QTabBar::mouseMoveEvent(event);
		return;
	}

	// If the left button isn't pressed anymore then return
	if (!(event->buttons() & Qt::LeftButton)) {
		return;
	}

	// If the distance is too small then return a urn.
	if ((pos - dragStartPos_).manhattanLength() < QApplication::startDragDistance()) {
		return;
	}

	// ok shit face nipple slapper fish.
	// lets check if your a kinky twat or just a rat.
	QRect tabgeo = tabRect(tabIdx_);

	// if the cursor is still in the tab get fooked !
	if (tabgeo.contains(pos)) {
		return;
	}

	// Ok now we know your not a smelly dog with 7 legs we can proceed.
	// we need to work out the direction o.o
	// well we get here when the mouse is outside the geo
	// dose qt get down with the chikens and provide me with < > ?
	DragResult result = DRAG_DETACH;

	int32_t num_tab = count();

	if (num_tab > 1)
	{
		if (pos.x() < tabgeo.left())
		{
			if (tabIdx_ != 0) {
				result = DRAG_MOVE_LEFT; // I could call move here but for now this makes easy debug.
			}
		}
		else if (pos.x() > tabgeo.right())
		{
			if (tabIdx_ != (num_tab - 1)) {
				result = DRAG_MOVE_RIGHT;
			}
		}
	}

	if (result == DRAG_DETACH && inMove_) {
		return;
	}

	if (result == DRAG_MOVE_LEFT)
	{
		inMove_ = true;

		int32_t width = tabRect(tabIdx_ - 1).width();
		tabgeo.moveLeft(tabgeo.left() - width);

		// mouse is less than right
		if (pos.x() > tabgeo.right()) {
			return;
		}

		moveTab(tabIdx_, tabIdx_ - 1);
		tabIdx_--;

		// tabIdx_ = -1;
	}
	else if (result == DRAG_MOVE_RIGHT) 
	{
		inMove_ = true;

		int32_t width = tabRect(tabIdx_ + 1).width();
		tabgeo.moveRight(tabgeo.right() + width);
		// all i care about is mouse is greater than left
		if (pos.x() < tabgeo.left()) {
			return;
		}

		moveTab(tabIdx_, tabIdx_ + 1);
		tabIdx_++;
	}
	else
	{
		event->accept();
		tabwidget_->detachTab(tabIdx_, tabOffset_);
	}
}

// ----------------------------------------------------------

CustomTabWidget::CustomTabWidget(QWidget * parent) :
	QTabWidget(parent)
{
	setTabBar((pTabBar_ = new CustomTabWidgetBar(this)));

	setTabsClosable(true);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setObjectName("codetab");
}


void CustomTabWidget::detachTab(int32_t index, QPoint pos)
{
	// we need to get the IEditor
	EditorView* view = qobject_cast<EditorView*>(parent());
	if (view) 
	{
		QWidget* widget = this->widget(index);
		IEditor* editor = view->tabWidgetToEditor(widget);

		if (editor) {
			EditorManager::undockEditor(editor, pos);
		}
	}
}



X_NAMESPACE_END