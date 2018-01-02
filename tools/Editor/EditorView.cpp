#include "stdafx.h"
#include "EditorView.h"

#include "IEditor.h"
#include "IAssetEntry.h"
#include "CustomTabWidget.h"
#include "EditorManager.h"
#include "BaseWindow.h"

X_NAMESPACE_BEGIN(editor)


DiamondBut::DiamondBut(QWidget* parent) : 
	QLabel(parent),
	active_(false)
{
}

void DiamondBut::setImage(QString filename)
{
	QPixmap img(filename);

	hover_ = img;

	QPainter p;
	p.begin(&img);
	p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
	p.fillRect(img.rect(), QColor(0, 0, 0, 150));
	p.end();

	img_ = img;

	this->setPixmap(img_);
}

void DiamondBut::setActive(bool b)
{
	if (active_ != b)
	{
		setPixmap(b ? hover_ : img_);
	}

	active_ = b;
}


// --------------------------------------------------------


Overlay::Overlay(QWidget * parent) :
	QWidget(parent),
	curButton_(CurButton::Invalid),
	lastButton_(CurButton::Invalid)
{
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);


	img_.load(":/dock/img/drag_box_main.png");

	// QColor overlay_col(8,33,58,192);
	QGridLayout* diamond = new QGridLayout;


	DiamondBut* center = new DiamondBut;
	center->setImage(":/dock/img/box_center.png");
	center->setMaximumSize(32, 32);

	DiamondBut* top = new DiamondBut;
	top->setImage(":/dock/img/dock_top.png");
	top->setMaximumSize(32, 32);

	DiamondBut* left = new DiamondBut;
	left->setImage(":/dock/img/dock_left.png");
	left->setMaximumSize(32, 32);

	DiamondBut* right = new DiamondBut;
	right->setImage(":/dock/img/dock_right.png");
	right->setMaximumSize(32, 32);

	DiamondBut* bottom = new DiamondBut;
	bottom->setImage(":/dock/img/dock_bottom.png");
	bottom->setMaximumSize(32, 32);

	buttons_[static_cast<int32_t>(CurButton::Top)] = top;
	buttons_[static_cast<int32_t>(CurButton::Left)] = left;
	buttons_[static_cast<int32_t>(CurButton::Right)] = right;
	buttons_[static_cast<int32_t>(CurButton::Bottom)] = bottom;
	buttons_[static_cast<int32_t>(CurButton::Center)] = center;

	diamond->setMargin(0);
	diamond->setSpacing(4);
	diamond->addWidget(center, 1, 1, 1, 1);
	diamond->addWidget(top, 0, 1, 1, 1);
	diamond->addWidget(left, 1, 0, 1, 1);
	diamond->addWidget(right, 1, 2, 1, 1);
	diamond->addWidget(bottom, 2, 1, 1, 1);


	QLabel* back = new QLabel;
	back->setPixmap(img_);
	back->setMaximumSize(128, 128);
	back->setLayout(diamond);
	back->setAttribute(Qt::WA_TranslucentBackground, true);

	QGridLayout* grid = new QGridLayout(this);
	grid->setMargin(0);
	grid->setSpacing(0);
	grid->addWidget(back, 1, 1);
}

Overlay::CurButton Overlay::getActiveButton(void)
{
	return curButton_;
}

void Overlay::updatePos(const QRect& rect)
{
	// check if mouse under any nipples.
	QPoint pos = QCursor::pos();

	curButton_ = CurButton::Invalid;

	for (int i = 0; i<5; i++)
	{
		DiamondBut* but = buttons_[i];
		QRect rec = but->rect();

		rec.moveTopLeft(but->mapToGlobal(rec.topLeft()));

		bool contains = rec.contains(pos);

		but->setActive(contains);

		if (contains)
		{
			// umm need to draw a overlay :)
			curButton_ = (CurButton)i;
		}
	}

	this->setGeometry(rect);
}

void Overlay::paintEvent(QPaintEvent *e)
{
	if (curButton_ == CurButton::Invalid /*|| m_lastButton == m_curButton*/) {
		e->accept();
		return;
	}

	lastButton_ = curButton_;

	QPainter p(this);
	QRect r = rect();

	p.setPen(QPen(QColor(80, 80, 80, 128), 6));
	p.setBrush(QBrush(QColor(32, 116, 220, 128)));

	if (curButton_ == CurButton::Center)
	{
		const int tab_height = 20;
		const int tab_width = 90;

		const QPoint points[6] = {
			QPoint(0, 0),
			QPoint(tab_width, 0),
			QPoint(tab_width, tab_height),
			QPoint(r.width(), tab_height),
			QPoint(r.width(), r.height()),
			QPoint(0, r.height()),
		};


		p.setRenderHint(QPainter::Antialiasing, true);
		p.drawPolygon(points, 6);
		update();
		return;
	}

	switch (curButton_)
	{
	case CurButton::Top:
		r.setHeight(r.height() / 2);
		break;
	case CurButton::Bottom:
		r.setHeight(r.height() / 2);
		r.moveTop(r.top() + r.height());
		break;
	case CurButton::Left:
		r.setWidth(r.width() / 2);
		break;
	case CurButton::Right:
		r.setWidth(r.width() / 2);
		r.moveLeft(r.left() + r.width());
		break;
	}

	//   qDebug() << "draw overlay: " << r;

	p.drawRect(r);
	update();
}




// --------------------------------------------------------


EditorView::EditorView(SplitterOrView *parentSplitterOrView, QWidget *parent) :
	QWidget(parent),
	pParentSplitterOrView_(parentSplitterOrView),
	pContainer_(new CustomTabWidget(this))
{
	setObjectName("EditorView");

	QVBoxLayout *tl = new QVBoxLayout(this);
	tl->setObjectName("EditorViewLayout");
	tl->setSpacing(0);
	tl->setMargin(0);
	tl->addWidget(pContainer_);

	pContainer_->setObjectName("EditorViewContainer");

	connect(pContainer_, SIGNAL(customContextMenuRequested(const QPoint &)),
		this, SLOT(showContextMenu(const QPoint &)));
	connect(pContainer_, SIGNAL(tabCloseRequested(int)),
		this, SLOT(tabCloseRequested(int)));
}

EditorView::~EditorView()
{
	qDebug() << "editor view deleted";
}


SplitterOrView* EditorView::parentSplitterOrView(void) const
{
	return pParentSplitterOrView_;
}


EditorView* EditorView::findNextView(void)
{
	SplitterOrView *current = parentSplitterOrView();
	BUG_ASSERT(current, return nullptr);
	SplitterOrView *parent = current->findParentSplitter();

	while (parent)
	{
		QSplitter *splitter = parent->splitter();
		BUG_ASSERT(splitter, return nullptr);
		BUG_ASSERT(splitter->count() == 2, return nullptr);
		// is current the first child? then the next view is the first one in current's sibling
		if (splitter->widget(0) == current) 
		{
			SplitterOrView *second = qobject_cast<SplitterOrView *>(splitter->widget(1));
			BUG_ASSERT(second, return nullptr);
			return second->findFirstView();
		}

		// otherwise go up the hierarchy
		current = parent;
		parent = current->findParentSplitter();
	}

	// current has no parent, so we are at the top and there is no "next" view
	return nullptr;
}


void EditorView::mousePressEvent(QMouseEvent *e)
{
	if (e->button() != Qt::LeftButton) {
		return;
	}
	setFocus(Qt::MouseFocusReason);
}


void EditorView::focusInEvent(QFocusEvent *)
{
	EditorManager::setCurrentView(this);
}


void EditorView::addEditor(IEditor *editor)
{
	if (editors_.contains(editor)) {
		return;
	}

	editors_.append(editor);

	editor->widget()->setObjectName("editor");

	pContainer_->addTab(editor->widget(), editor->assetEntry()->displayName());

	widgetEditorMap_.insert(editor->widget(), editor);

	connect(editor, SIGNAL(titleChanged(const QString &)), this, SLOT(titleChanged(const QString &)));

	if (editor == currentEditor()) {
		setCurrentEditor(editor);
	}
}

bool EditorView::hasEditor(IEditor *editor) const
{
	return editors_.contains(editor);
}

void EditorView::removeEditor(IEditor *editor)
{
	BUG_ASSERT(editor, return);
	if (!editors_.contains(editor)) {
		return;
	}

	disconnect(editor, SIGNAL(titleChanged(const QString &)), this, SLOT(titleChanged(const QString &)));

	const int32_t index = pContainer_->indexOf(editor->widget());
	BUG_ASSERT((index != -1), return);
	bool wasCurrent = (index == pContainer_->currentIndex());
	editors_.removeAll(editor);

	pContainer_->removeTab(index);
	widgetEditorMap_.remove(editor->widget());
	editor->widget()->setParent(0);

	if (wasCurrent) {
		setCurrentEditor(editors_.count() ? editors_.last() : 0);
	}
}

IEditor *EditorView::currentEditor(void) const
{
	if (editors_.count() > 0) {
		return widgetEditorMap_.value(pContainer_->currentWidget());
	}
	return nullptr;
}


void EditorView::setCurrentEditor(IEditor *editor)
{
	if (!editor || pContainer_->indexOf(editor->widget()) == -1) {
		BUG_CHECK(!editor);
		pContainer_->setCurrentIndex(0);
		return;
	}

	editors_.removeAll(editor);
	editors_.append(editor);

	const int32_t idx = pContainer_->indexOf(editor->widget());
	BUG_ASSERT(idx >= 0, return);
	pContainer_->setCurrentIndex(idx);
}

int32_t EditorView::editorCount(void) const
{
	return editors_.size();
}

QList<IEditor*> EditorView::editors(void) const
{
	return editors_;
}

IEditor* EditorView::tabWidgetToEditor(QWidget* pWidget)
{
	return widgetEditorMap_.value(pWidget);
}

IEditor* EditorView::editorForAssetEntry(const IAssetEntry* pAssetEntry) const
{
	for(IEditor* pEditor : editors_) {
		if (pEditor->assetEntry() == pAssetEntry) {
			return pEditor;
		}
	}
	return nullptr;
}


void EditorView::closeView(void)
{
	IEditor* pEditor = currentEditor();
	if (pEditor) {
		EditorManager::closeEditor(pEditor);
	}
}

void EditorView::splitHorizontally(void)
{
	if (pParentSplitterOrView_) {
		pParentSplitterOrView_->split(Qt::Vertical);
	}
	EditorManager::updateActions();
}

void EditorView::splitVertically(void)
{
	if (pParentSplitterOrView_) {
		pParentSplitterOrView_->split(Qt::Horizontal);
	}
	EditorManager::updateActions();
}


void EditorView::splitNewWindow(void)
{
	EditorManager::splitNewWindow(this);
}

void EditorView::closeSplit(void)
{
	EditorManager::closeView(this);
	EditorManager::updateActions();
}


void EditorView::setParentSplitterOrView(SplitterOrView *splitterOrView)
{
	pParentSplitterOrView_ = splitterOrView;
}

void EditorView::showContextMenu(const QPoint& point)
{
	if (point.isNull()) {
		return;
	}

	int32_t idx = pContainer_->tabBar()->tabAt(point);

	qDebug() << "TabContex idx " << idx;

	if (idx >= 0)
	{
		QWidget* tab = pContainer_->widget(idx);
		IEditor *editor = widgetEditorMap_.value(tab);

		if (editor)
		{
			QMenu menu;

			EditorManager::addSaveAndCloseEditorActions(&menu, editor);
			menu.addSeparator();
			EditorManager::addNativeDirActions(&menu, editor);
			menu.addSeparator();
			EditorManager::addFloatActions(&menu, editor);

			menu.exec(pContainer_->tabBar()->mapToGlobal(point));
		}
	}
}


void EditorView::tabCloseRequested(int32_t idx)
{
	QWidget* tab = pContainer_->widget(idx);
	IEditor *editor = widgetEditorMap_.value(tab);


	// if this was the last tab close the view.
	int32_t count = pContainer_->count();
	//   qDebug() << "remaing tabs: " << count;
	if (count == 0)
	{
		//        closeSplit();
	}

	EditorManager::closeEditor(editor);
}

void EditorView::titleChanged(QString title)
{
	IEditor* editor = qobject_cast<IEditor*>(sender());

	if (editor)
	{
		int32_t idx = pContainer_->indexOf(editor->widget());

		if (idx >= 0)
		{
			pContainer_->setTabText(idx, title);
		}
	}
}


// --------------------------------------------------------

SplitterOrView::SplitterOrView(BaseWindow* pWindow)
{
	dragging_ = false;
	floated_ = true;
	pLayout_ = new QStackedLayout(this);
	pLayout_->setSizeConstraint(QLayout::SetNoConstraint);
	pView_ = new EditorView(this, pWindow);

	pSplitter_ = nullptr;
	pLayout_->addWidget(pView_);

	pWindow->setCentralWidget(this);

}

SplitterOrView::SplitterOrView(IEditor *editor)
{
	dragging_ = false;
	floated_ = false;
	pLayout_ = new QStackedLayout(this);
	pLayout_->setSizeConstraint(QLayout::SetNoConstraint);
	pView_ = new EditorView(this);
	if (editor) {
		pView_->addEditor(editor);
	}
	pSplitter_ = nullptr;
	pLayout_->addWidget(pView_);
}

SplitterOrView::SplitterOrView(EditorView *view)
{
	BUG_CHECK(view);
	dragging_ = false;
	floated_ = false;
	pLayout_ = new QStackedLayout(this);
	pLayout_->setSizeConstraint(QLayout::SetNoConstraint);
	pView_ = view;
	pView_->setParentSplitterOrView(this);
	pSplitter_ = nullptr;
	pLayout_->addWidget(pView_);
}

SplitterOrView::~SplitterOrView()
{
	delete pLayout_;
	pLayout_ = nullptr;
	if (pView_) {
		EditorManager::emptyView(pView_);
	}
	delete pView_;
	pView_ = nullptr;
	delete pSplitter_;
	pSplitter_ = nullptr;
}

EditorView *SplitterOrView::findFirstView(void)
{
	if (pSplitter_) {
		for (int32_t i = 0; i < pSplitter_->count(); ++i) {
			if (SplitterOrView *splitterOrView = qobject_cast<SplitterOrView*>(pSplitter_->widget(i)))
				if (EditorView *result = splitterOrView->findFirstView()) {
					return result;
				}
		}
		return nullptr;
	}
	return pView_;
}

SplitterOrView *SplitterOrView::findParentSplitter(void) const
{
	QWidget *w = parentWidget();
	while (w) {
		if (SplitterOrView *splitter = qobject_cast<SplitterOrView *>(w)) {
			BUG_CHECK(splitter->splitter());
			return splitter;
		}
		w = w->parentWidget();
	}
	return nullptr;
}

QSize SplitterOrView::minimumSizeHint(void) const
{
	if (pSplitter_) {
		return pSplitter_->minimumSizeHint();
	}
	return QSize(64, 64);
}

QSplitter* SplitterOrView::takeSplitter(void)
{
	QSplitter* oldSplitter = pSplitter_;
	if (pSplitter_) {
		pLayout_->removeWidget(pSplitter_);
	}
	pSplitter_ = nullptr;
	return oldSplitter;
}

EditorView* SplitterOrView::takeView(void)
{
	EditorView* oldView = pView_;
	if (pView_) {
		// the focus update that is triggered by removing should already have 0 parent
		// so we do that first
		pView_->setParentSplitterOrView(0);
		pLayout_->removeWidget(pView_);
	}
	pView_ = nullptr;
	return oldView;
}

void SplitterOrView::split(Qt::Orientation orientation)
{
	Q_ASSERT(pView_ && pSplitter_ == 0);
	pSplitter_ = new QSplitter(this);
	pSplitter_->setOrientation(orientation);
	pLayout_->addWidget(pSplitter_);
	pLayout_->removeWidget(pView_);
	EditorView *editorView = pView_;
	pView_ = nullptr;
	IEditor *e = editorView->currentEditor();

	SplitterOrView* view = nullptr;
	SplitterOrView* otherView = nullptr;
	IEditor* duplicate = e && e->duplicateSupported() ? EditorManager::duplicateEditor(e) : nullptr;
	pSplitter_->addWidget((view = new SplitterOrView(duplicate)));
	pSplitter_->addWidget((otherView = new SplitterOrView(editorView)));


	pLayout_->setCurrentWidget(pSplitter_);

	view->view()->setCurrentEditor(duplicate);

	if (e) {
		EditorManager::activateEditor(otherView->view(), e);
	}
	else {
		EditorManager::setCurrentView(otherView->view());
	}
}


void SplitterOrView::moveEditor(Qt::Orientation orientation, IEditor *editor, Direction side)
{
	Q_ASSERT(pView_ && pSplitter_ == nullptr);

	// make sure this editor is part of the view.
	if (!pView_->hasEditor(editor)) {
		return;
	}

	// remove the editor.
	pView_->removeEditor(editor);

	pSplitter_ = new QSplitter(this);
	pSplitter_->setOrientation(orientation);
	pLayout_->addWidget(pSplitter_);
	pLayout_->removeWidget(pView_);
	EditorView *editorView = pView_;
	pView_ = 0;

	SplitterOrView *otherView = nullptr;

	int32_t idx = 0;
	if (side == Direction::Default || side == Direction::Right || side == Direction::Bottom) {
		idx ^= 1;
	}

	pSplitter_->insertWidget(idx ^ 1, new SplitterOrView(editorView));
	pSplitter_->insertWidget(idx, (otherView = new SplitterOrView(editor)));


	pLayout_->setCurrentWidget(pSplitter_);

	otherView->view()->setCurrentEditor(editor);

	EditorManager::activateEditor(otherView->view(), editor);
}

void SplitterOrView::addSplitter(SplitterOrView* split, Qt::Orientation orientation, Direction side)
{
	BUG_ASSERT(isView(), return); // this should only be called on view since you can only drag&drop onto a view.

	// ok so basically we wanna just split
	// but we already have the new view.

	pSplitter_ = new QSplitter(this);
	pSplitter_->setOrientation(orientation);
	pLayout_->addWidget(pSplitter_);
	pLayout_->removeWidget(pView_);
	EditorView *editorView = pView_;
	pView_ = nullptr;


	int32_t idx = 0;
	if (side == Direction::Default || side == Direction::Right || side == Direction::Bottom) {
		idx ^= 1;
	}

	pSplitter_->insertWidget(idx ^ 1, new SplitterOrView(editorView));
	pSplitter_->insertWidget(idx, split);

	// make it 50%
	QList<int32_t> sizes = pSplitter_->sizes();

	int32_t size = (sizes[0] + sizes[1]) / 2;
	sizes[0] = size;
	sizes[1] = size;

	pSplitter_->setSizes(sizes);

	pLayout_->setCurrentWidget(pSplitter_);
}

void SplitterOrView::unsplitAll(void)
{
	BUG_ASSERT(pSplitter_, return);
	// avoid focus changes while unsplitting is in progress
	bool hadFocus = false;
	if (QWidget* w = focusWidget()) {
		if (w->hasFocus()) {
			w->clearFocus();
			hadFocus = true;
		}
	}

	EditorView* currentView = EditorManager::currentEditorView();
	if (currentView) {
		currentView->parentSplitterOrView()->takeView();
		currentView->setParentSplitterOrView(this);
	}
	else {
		currentView = new EditorView(this);
	}

	pSplitter_->hide();
	pLayout_->removeWidget(pSplitter_); // workaround Qt bug
	unsplitAllHelper();
	pView_ = currentView;
	pLayout_->addWidget(pView_);
	delete pSplitter_;
	pSplitter_ = nullptr;

	// restore some focus
	if (hadFocus) {
		if (IEditor* editor = pView_->currentEditor()) {
			editor->widget()->setFocus();
		}
		else {
			pView_->setFocus();
		}
	}
}

void SplitterOrView::unsplitAllHelper(void)
{
	if (pView_) {
		EditorManager::emptyView(pView_);
	}

	if (pSplitter_) {
		for (int32_t i = 0; i < pSplitter_->count(); ++i) {
			if (SplitterOrView *splitterOrView = qobject_cast<SplitterOrView*>(pSplitter_->widget(i))) {
				splitterOrView->unsplitAllHelper();
			}
		}
	}
}


void SplitterOrView::unsplit(void)
{
	if (!pSplitter_) {
		return;
	}

	SplitterOrView* childSplitterOrView = qobject_cast<SplitterOrView*>(pSplitter_->widget(0));
	QSplitter* oldSplitter = pSplitter_;
	pSplitter_ = nullptr;

	if (childSplitterOrView->isSplitter()) {
		Q_ASSERT(childSplitterOrView->view() == 0);
		pSplitter_ = childSplitterOrView->takeSplitter();
		pLayout_->addWidget(pSplitter_);
		pLayout_->setCurrentWidget(pSplitter_);
	}
	else
	{
		EditorView* childView = childSplitterOrView->view();
		Q_ASSERT(childView);
		if (pView_) {
			//        pView_->copyNavigationHistoryFrom(childView);
			if (IEditor *e = childView->currentEditor()) {
				childView->removeEditor(e);
				pView_->addEditor(e);
				pView_->setCurrentEditor(e);
			}
			EditorManager::emptyView(childView);
		}
		else {
			pView_ = childSplitterOrView->takeView();
			pView_->setParentSplitterOrView(this);
			pLayout_->addWidget(pView_);
		}
		pLayout_->setCurrentWidget(pView_);
	}

	delete oldSplitter;
	EditorManager::setCurrentView(findFirstView());
}


void SplitterOrView::unsplit(SplitterOrView *view_to_remove)
{
	if (!pSplitter_) {
		return;
	}

	if (!view_to_remove->isView()) {
		return;
	}

	// for now.
	if (pSplitter_->count() != 2) {
		return;
	}

	// we want to remove the view
	// delete the splitter
	// and set hte pView_.

	int32_t index = pSplitter_->indexOf(view_to_remove);
	index ^= 1;

	SplitterOrView *ViewToKeep = qobject_cast<SplitterOrView*>(pSplitter_->widget(index));

	ViewToKeep->setObjectName("viewToKeep");
	view_to_remove->setObjectName("viewToRemove");

	// we want to remove this.
	QSplitter* oldSplitter = pSplitter_;
	pSplitter_ = nullptr;

	// take the view and place it.
	pView_ = ViewToKeep->takeView();
	pView_->setParentSplitterOrView(this);
	pLayout_->addWidget(pView_);
	pLayout_->setCurrentWidget(pView_);

	delete oldSplitter;
	EditorManager::setCurrentView(findFirstView());
}

void SplitterOrView::SetDragMode(QPoint& pos)
{
	dragging_ = true;
	tabOffset_ = pos;
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
}

void SplitterOrView::dragMakeNewWindow(void)
{
	setAttribute(Qt::WA_TranslucentBackground, false);
	setWindowFlags((windowFlags() & ~Qt::FramelessWindowHint));

	// ok we got a kinky view.
	// tickle my pool.
}

void SplitterOrView::mouseMoveEvent(QMouseEvent* event)
{
	if (dragging_) {
		move(event->globalPos() - tabOffset_);

		// check with editor mangager if we are floating baby !
		EditorManager::floatDockCheck(this, event->globalPos());
	}
}

void SplitterOrView::mouseReleaseEvent(QMouseEvent*)
{
	if (dragging_) {
		EditorManager::splitDragEnd(this);
	}
}








X_NAMESPACE_END