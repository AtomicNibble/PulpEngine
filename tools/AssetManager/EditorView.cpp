#include "stdafx.h"
#include "EditorView.h"

#include "IEditor.h"
#include "IAssetEntry.h"
#include "CustomTabWidget.h"
#include "EditorManager.h"

X_NAMESPACE_BEGIN(assman)


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
	connect(pContainer_, SIGNAL(tabCloseRequested(int32_t)),
		this, SLOT(tabCloseRequested(int32_t)));
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
	pSplitter_ = 0;
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
	pSplitter_ = 0;
	pLayout_->addWidget(pView_);
}

SplitterOrView::~SplitterOrView()
{
	delete pLayout_;
	pLayout_ = 0;
	if (pView_) {
		EditorManager::emptyView(pView_);
	}
	delete pView_;
	pView_ = 0;
	delete pSplitter_;
	pSplitter_ = 0;
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

	SplitterOrView *otherView = 0;

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
	pView_ = 0;


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

void SplitterOrView::unsplitAll()
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

void SplitterOrView::dragMakeNewWindow()
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