#include "stdafx.h"
#include "EditorManager.h"

#include "IAssetEntry.h"
#include "IEditor.h"
#include "IEditorFactory.h"

#include "EditorView.h"
#include "CustomTabWidget.h"

#include "ActionManager.h"
#include "ActionContainer.h"
#include "Command.h"

#include "AssetEntryModel.h"
#include "AssetEntryManager.h"

#include "DirectionConstants.h"
#include "BaseWindow.h"

X_NAMESPACE_BEGIN(editor)


namespace 
{

	template <class EditorFactoryLike>
	X_INLINE EditorFactoryLike* findById(const Id &id)
	{
		const QList<EditorFactoryLike *>& factories = d->factories_;
		foreach(EditorFactoryLike *efl, factories) {
			if (id == efl->id()) {
				return efl;
			}
		}

		return nullptr;
	}



	class EditorManagerPrivate
	{
	public:
		explicit EditorManagerPrivate(QWidget *parent);
		~EditorManagerPrivate();

	public:
		QList<SplitterOrView *> root_;
		QList<IContext *> rootContext_;

		QPointer<IEditor> currentEditor_;
		QPointer<IEditor> scheduledCurrentEditor_;
		QPointer<EditorView> currentView_;

		QTimer* autoSaveTimer_;

		// actions
		QAction* revertToSavedAction_;
		QAction* saveAction_;
		QAction* saveAsAction_;
		QAction* closeCurrentEditorAction_;
		QAction* closeAllEditorsAction_;
		QAction* closeOtherEditorsAction_;
		IEditor* contextMenuEntry_;

		// context actions
		QAction* saveCurrentEditorContextAction_;
		QAction* closeCurrentEditorContextAction_;
		QAction* closeAllEditorsContextAction_;
		QAction* closeOtherEditorsContextAction_;
		QAction* copyFullPathContextAction_;

		QAction* FloatEditorContextAction_;
		QAction* DockEditorMainContextAction_;
		QAction* moveToNewHozTabGroupContextAction_;
		QAction* moveToNewVerTabGroupContextAction_;

		QAction* splitHozTabGroupContextAction_;
		QAction* splitVerTabGroupContextAction_;


		// sexy split shit
		QAction* splitAction_;
		QAction* splitSideBySideAction_;
		QAction* splitNewWindowAction_;
		QAction* removeCurrentSplitAction_;
		QAction* removeAllSplitsAction_;

		QString titleAddition_;
		QString titleVcsTopic_;

		// QMap<QString, QVariant> editorStates_;

		EditorManager::EditorFactoryList factories_;
		AssetEntryModel* pAssetEntryModel_;;

		QPointer<Overlay> overlay_;
		QPointer<EditorView> dropTargetView_;

		int32_t autoSaveInterval_;
		bool autoSaveEnabled_;
	};


	EditorManagerPrivate::EditorManagerPrivate(QWidget *parent) :
		autoSaveTimer_(0),
		revertToSavedAction_(new QAction(EditorManager::tr("Revert to Saved"), parent)),
		saveAction_(new QAction(parent)),
		saveAsAction_(new QAction(parent)),
		closeCurrentEditorAction_(new QAction(EditorManager::tr("Close"), parent)),
		closeAllEditorsAction_(new QAction(EditorManager::tr("Close All Editors"), parent)),
		closeOtherEditorsAction_(new QAction(EditorManager::tr("Close All But This"), parent)),

		saveCurrentEditorContextAction_(new QAction(QIcon(":/misc/img/Savea.png"), EditorManager::tr("&Save"), parent)),
		closeCurrentEditorContextAction_(new QAction(QIcon(":/misc/img/quit.png"), EditorManager::tr("Close"), parent)),
		closeAllEditorsContextAction_(new QAction(EditorManager::tr("Close All Editors"), parent)),
		closeOtherEditorsContextAction_(new QAction(EditorManager::tr("Close All But This"), parent)),
		copyFullPathContextAction_(new QAction(EditorManager::tr("Copy Full Path"), parent)),

		FloatEditorContextAction_(new QAction(EditorManager::tr("Float to New Window"), parent)),
		DockEditorMainContextAction_(new QAction("Dock To Main View", parent)),

		moveToNewHozTabGroupContextAction_(new QAction("Move to New Horizontal Tab Group", parent)),
		moveToNewVerTabGroupContextAction_(new QAction("Move to New Vertical Tab Group", parent)),

		splitHozTabGroupContextAction_(new QAction("Duplicate In New Horizontal Tab Group", parent)),
		splitVerTabGroupContextAction_(new QAction("Duplicate In New Vertical Tab Group", parent)),

		autoSaveEnabled_(false),
		autoSaveInterval_(5),
		currentEditor_(nullptr),
		currentView_(nullptr),
		pAssetEntryModel_(nullptr)
	{

		pAssetEntryModel_ = new AssetEntryModel(parent);
		overlay_ = new Overlay();
	}

	EditorManagerPrivate::~EditorManagerPrivate()
	{

	}

	static EditorManager* pInstance_ = nullptr;
	static EditorManagerPrivate* d = nullptr;

} // namespace



EditorManager* EditorManager::instance(void) 
{ 
	return pInstance_; 
}

EditorManager::EditorManager(QWidget *parent) :
	QWidget(parent)
{
	d = new EditorManagerPrivate(parent);
	pInstance_ = this;

	setObjectName("EditorView");

	connect(ICore::instance(), SIGNAL(contextAboutToChange(QList<IContext*>)),
		this, SLOT(handleContextChange(QList<IContext*>)));

	const Context editManagerContext(Constants::C_EDITORMANAGER);

	// Save Action
	ActionManager::registerAction(d->saveAction_, Constants::SAVE, editManagerContext);
	connect(d->saveAction_, SIGNAL(triggered()), this, SLOT(saveAssetEntry()));

	// Save As Action
	ActionManager::registerAction(d->saveAsAction_, Constants::SAVEAS, editManagerContext);
	connect(d->saveAsAction_, SIGNAL(triggered()), this, SLOT(saveAssetEntryAs()));


	// Window Menu
	ActionContainer* mwindow = ActionManager::actionContainer(Constants::M_WINDOW);

	// Window menu separators
	mwindow->addSeparator(editManagerContext, Constants::G_WINDOW_SPLIT);


	d->splitAction_ = new QAction(tr("Split"), this);
	ICommand *cmd = ActionManager::registerAction(d->splitAction_, Constants::SPLIT, editManagerContext);
	cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+E,2")));
	mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
	connect(d->splitAction_, SIGNAL(triggered()), this, SLOT(split()));

	d->splitSideBySideAction_ = new QAction(tr("Split Side by Side"), this);
	cmd = ActionManager::registerAction(d->splitSideBySideAction_, Constants::SPLIT_SIDE_BY_SIDE, editManagerContext);
	cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+E,3")));
	mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
	connect(d->splitSideBySideAction_, SIGNAL(triggered()), this, SLOT(splitSideBySide()));

	d->splitNewWindowAction_ = new QAction(tr("Float to New Window"), this);
	cmd = ActionManager::registerAction(d->splitNewWindowAction_, Constants::SPLIT_NEW_WINDOW, editManagerContext);
	cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+E,4")));
	mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
	connect(d->splitNewWindowAction_, SIGNAL(triggered()), this, SLOT(splitNewWindow()));

	d->removeCurrentSplitAction_ = new QAction(tr("Remove Current Split"), this);
	cmd = ActionManager::registerAction(d->removeCurrentSplitAction_, Constants::REMOVE_CURRENT_SPLIT, editManagerContext);
	cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+E,0")));
	mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
	connect(d->removeCurrentSplitAction_, SIGNAL(triggered()), this, SLOT(removeCurrentSplit()));

	d->removeAllSplitsAction_ = new QAction(tr("Remove All Splits"), this);
	cmd = ActionManager::registerAction(d->removeAllSplitsAction_, Constants::REMOVE_ALL_SPLITS, editManagerContext);
	cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+E,1")));
	mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
	connect(d->removeAllSplitsAction_, SIGNAL(triggered()), this, SLOT(removeAllSplits()));


	//Save XXX Context Actions
	connect(d->saveCurrentEditorContextAction_, SIGNAL(triggered()), this, SLOT(saveAssetEntryFromContextMenu()));

	// Close XXX Context Actions
	connect(d->closeAllEditorsContextAction_, SIGNAL(triggered()), this, SLOT(closeAllEditors()));
	connect(d->closeCurrentEditorContextAction_, SIGNAL(triggered()), this, SLOT(closeEditorFromContextMenu()));
	connect(d->closeOtherEditorsContextAction_, SIGNAL(triggered()), this, SLOT(closeOtherEditorsFromContextMenu()));

	// shizz
	connect(d->copyFullPathContextAction_, SIGNAL(triggered()), this, SLOT(copyFullPath()));

	// float
	connect(d->FloatEditorContextAction_, SIGNAL(triggered()), this, SLOT(floatEditor()));
	connect(d->DockEditorMainContextAction_, SIGNAL(triggered()), this, SLOT(dockEditorMain()));

	// Move to a new group (NOT duplicated)
	connect(d->moveToNewHozTabGroupContextAction_, SIGNAL(triggered()), this, SLOT(moveNewHozTabGroup()));
	connect(d->moveToNewVerTabGroupContextAction_, SIGNAL(triggered()), this, SLOT(moveNewVerTabGroup()));

	// Duplication (splits)
	connect(d->splitHozTabGroupContextAction_, SIGNAL(triggered()), this, SLOT(newHozTabGroup()));
	connect(d->splitVerTabGroupContextAction_, SIGNAL(triggered()), this, SLOT(newVerTabGroup()));



	ActionContainer* mfile = ActionManager::actionContainer(Constants::M_FILE);

	connect(mfile->menu(), SIGNAL(aboutToShow()), pInstance_, SLOT(updateActions()));


	IContext *context = new IContext;
	context->setContext(Context(Constants::C_EDITORMANAGER));
	context->setWidget(this);
	ICore::addContextObject(context);

	// other setup
	SplitterOrView *firstRoot = new SplitterOrView();
	d->root_.append(firstRoot);
	d->rootContext_.append(context);
	d->currentView_ = firstRoot->view();


	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(firstRoot);


	updateActions();

	d->autoSaveTimer_ = new QTimer(this);
	connect(d->autoSaveTimer_, SIGNAL(timeout()), SLOT(autoSave()));
	updateAutoSave();
}

EditorManager::~EditorManager(void)
{
	pInstance_ = nullptr;


	// close all extra windows
	for (int32_t i = 1; i < d->root_.size(); ++i)
	{
		SplitterOrView *root = d->root_.at(i);
		disconnect(root, SIGNAL(destroyed(QObject*)), this, SLOT(rootDestroyed(QObject*)));
		IContext *rootContext = d->rootContext_.at(i);
		ICore::removeContextObject(rootContext);
		delete root;
		delete rootContext;
	}

	d->root_.clear();
	d->rootContext_.clear();

	delete d;
}


void EditorManager::AddFactory(IEditorFactory* factory)
{
	d->factories_.push_back(factory);


}


void EditorManager::init(void)
{



}

void EditorManager::updateAutoSave(void)
{
	if (d->autoSaveEnabled_) {
		d->autoSaveTimer_->start(d->autoSaveInterval_ * (60 * 1000));
	}
	else {
		d->autoSaveTimer_->stop();
	}
}



SplitterOrView *EditorManager::findRoot(const EditorView* pView, int32_t* pRootIndex)
{
	SplitterOrView* pCurrent = pView->parentSplitterOrView();
	while (pCurrent)
	{
		int32_t index = d->root_.indexOf(pCurrent);
		if (index >= 0) {
			if (pRootIndex) {
				*pRootIndex = index;
			}
			return pCurrent;
		}
		pCurrent = pCurrent->findParentSplitter();
	}
	BUG_CHECK(false); // we should never have views without a root
	return nullptr;
}


QByteArray EditorManager::saveState(void)
{
	QByteArray bytes;
	QDataStream stream(&bytes, QIODevice::WriteOnly);

	stream << QByteArray("EditorManager");

	auto entries = d->pAssetEntryModel_->assetEntrys();
	int entriesCount = 0;
	foreach(AssetEntryModel::Entry* entry, entries) {
		// The editor may be 0 if it was not loaded yet: In that case it is not temporary
		if (!entry->pAssetEntry_->isTemporary()) {
			++entriesCount;
		}
	}

	stream << entriesCount;

	foreach(AssetEntryModel::Entry* entry, entries) {
		if (!entry->pAssetEntry_->isTemporary()) {
			stream << entry->assetName() << entry->type() << entry->id();
		}
	}


	return bytes;
}

bool EditorManager::restoreState(const QByteArray& state)
{
	closeAllEditors(true);


	QDataStream stream(state);

	QByteArray version;
	stream >> version;

	if (version != "EditorManager") {
		return false;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);

	int32_t editorCount = 0;
	stream >> editorCount;
	while (--editorCount >= 0) 
	{
		QString name;
		stream >> name;
		int type;
		stream >> type;
		Id id;
		stream >> id;

		if (!name.isEmpty())
		{
			openEditor(name, static_cast<assetDb::AssetType::Enum>(type), id, DoNotMakeVisible);
		}
	}

	QApplication::restoreOverrideCursor();
	return true;
}


IAssetEntry* EditorManager::currentAssetEntry(void)
{
	return d->currentEditor_ ? d->currentEditor_->assetEntry() : 0;
}

IEditor *EditorManager::currentEditor(void)
{
	return d->currentEditor_;
}

QList<IEditor*> EditorManager::visibleEditors(void)
{
	QList<IEditor *> editors;
	for(SplitterOrView* pRoot : d->root_) 
	{
		if (pRoot->isSplitter())
		{
			EditorView* firstView = pRoot->findFirstView();
			EditorView* view = firstView;
			if (view) 
			{
				do {
					if (view->currentEditor()) {
						editors.append(view->currentEditor());
					}
					view = view->findNextView();
					BUG_ASSERT(view != firstView, break); // we start with firstView and shouldn't have cycles
				} while (view);
			}
		}
		else {
			if (pRoot->editor()) {
				editors.append(pRoot->editor());
			}
		}
	}
	return editors;
}

QList<EditorView*> EditorManager::visibleViews(void)
{
	QList<EditorView *> views;
	for (SplitterOrView* pRoot : d->root_)
	{
		if (pRoot->isSplitter())
		{
			EditorView *firstView = pRoot->findFirstView();
			EditorView *view = firstView;
			if (view) 
			{
				do {
					views.append(view);
					view = view->findNextView();
				} while (view);
			}
		}
		else
		{
			if (pRoot->isView()) {
				views.append(pRoot->view());
			}
		}
	}
	return views;
}



QList<IEditor*> EditorManager::openEditorsList(void)
{
	QList<IEditor *> editors;
	for (SplitterOrView* pRoot : d->root_)
	{
		if (pRoot->isSplitter()) 
		{
			EditorView *firstView = pRoot->findFirstView();
			EditorView *view = firstView;

			if (view) 
			{
				do {
					editors.append(view->editors());
					view = view->findNextView();

				} while (view);
			}

		}
		else {
			editors.append(pRoot->editors());
		}
	}
	return editors;
}



IEditor *EditorManager::openEditor(const QString &assetName, assetDb::AssetType::Enum type, const Id &editorId,
	OpenEditorFlags flags, bool *newEditor)
{
	return pInstance_->openEditor(pInstance_->currentEditorView(),
		assetName, type, editorId, flags, newEditor);
}


void EditorManager::addEditor(IEditor *editor)
{
	if (!editor) {
		return;
	}

	ICore::addContextObject(editor);

	bool isNewAssetEntry = false;
	d->pAssetEntryModel_->addEditor(editor, &isNewAssetEntry);
	if (isNewAssetEntry)
	{
		const bool isTemporary = editor->assetEntry()->isTemporary();
		AssetEntryManager::addAssetEntry(editor->assetEntry());
		if (!isTemporary) {
			const auto pEntry = editor->assetEntry();
			AssetEntryManager::addToRecentFiles(pEntry->name(), pEntry->type(), editor->id());
		}
	}
	emit pInstance_->editorOpened(editor);
}

void EditorManager::removeEditor(IEditor *editor)
{
	bool lastOneForDocument = false;
	d->pAssetEntryModel_->removeEditor(editor, &lastOneForDocument);
	if (lastOneForDocument) {
		AssetEntryManager::removeAssetEntry(editor->assetEntry());
	}

	ICore::removeContextObject(editor);
}


void EditorManager::handleContextChange(const QList<IContext *> &context)
{
	//    if (debugLogging)
	//        qDebug() << Q_FUNC_INFO;

	d->scheduledCurrentEditor_ = 0;
	IEditor *editor = nullptr;
	foreach(IContext *c, context) 
	{
		if ((editor = qobject_cast<IEditor*>(c))) {
			break;
		}
	}

	if (editor && editor != d->currentEditor_) {
		// Delay actually setting the current editor to after the current event queue has been handled
		// Without doing this, e.g. clicking into projects tree or locator would always open editors
		// in the main window. That is because clicking anywhere in the main window (even over e.g.
		// the locator line edit) first activates the window and sets focus to its focus widget.
		// Only afterwards the focus is shifted to the widget that received the click.
		//    qDebug() << "editor changed";
		d->scheduledCurrentEditor_ = editor;
		QTimer::singleShot(0, pInstance_, SLOT(setCurrentEditorFromContextChange()));
	}
	else {
		updateActions();
	}
}

void EditorManager::setCurrentEditorFromContextChange(void)
{
	if (!d->scheduledCurrentEditor_) {
		return;
	}

	IEditor *newCurrent = d->scheduledCurrentEditor_;
	d->scheduledCurrentEditor_ = nullptr;
	setCurrentEditor(newCurrent);
}


EditorView* EditorManager::currentEditorView(void)
{
	EditorView* view = d->currentView_;
	if (!view) 
	{
		if (d->currentEditor_) {
			view = viewForEditor(d->currentEditor_);
			BUG_ASSERT(view, view = d->root_.first()->findFirstView());
		}
		BUG_CHECK(view);
		if (!view) { // should not happen, we should always have either currentview or currentAssetEntry
			foreach(SplitterOrView *root, d->root_) {
				if (root->window()->isActiveWindow()) {
					view = root->findFirstView();
					break;
				}
			}
			BUG_ASSERT(view, view = d->root_.first()->findFirstView());
		}
	}
	return view;
}


void EditorManager::setAutoSaveEnabled(bool enabled)
{
	d->autoSaveEnabled_ = enabled;
	updateAutoSave();
}

bool EditorManager::autoSaveEnabled(void)
{
	return d->autoSaveEnabled_;
}

void EditorManager::setAutoSaveInterval(int32_t interval)
{
	d->autoSaveInterval_ = interval;
	updateAutoSave();
}

int32_t EditorManager::autoSaveInterval(void)
{
	return d->autoSaveInterval_;
}


IEditor *EditorManager::openEditor(EditorView* pView, const QString& assetName, assetDb::AssetType::Enum type,
	const Id &editorId, OpenEditorFlags flags, bool* pNewEditor)
{
	if (debugLogging) {
		qDebug() << Q_FUNC_INFO << assetName << editorId.name();
	}

	if (assetName.isEmpty()) {
		X_WARNING("Editor", "Can't open editor for empty name");
		return nullptr;
	}
	if (pNewEditor) {
		*pNewEditor = false;
	}

	// see if edtiro already open for this
	const QList<IEditor*> editors = d->pAssetEntryModel_->editorsForAsset(assetName, type);
	if (!editors.isEmpty()) {
		IEditor* pEditor = editors.first();
		pEditor = activateEditor(pView, pEditor, flags);
		return pEditor;
	}


	IEditor* pEditor = createEditor(editorId, assetName);
	if (!pEditor)
	{
		X_ERROR("Editor", "Failed to createEditor for: \"%s\"", qPrintable(assetName));
		return nullptr;
	}

	IEditor* pResult = nullptr;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	{
		QString errorString;
		if (!pEditor->open(&errorString, assetName, type))
		{
			QApplication::restoreOverrideCursor();
			QMessageBox::critical(ICore::mainWindow(), tr("File Error"), errorString);
			delete pEditor;
			return nullptr;
		}

		addEditor(pEditor);

		if (pNewEditor) {
			*pNewEditor = true;
		}

		pResult = activateEditor(pView, pEditor, flags);
	}
	QApplication::restoreOverrideCursor();

	return pResult;
}


void EditorManager::activateEditor(IEditor *editor, OpenEditorFlags flags)
{
	BUG_ASSERT(editor, return);
	EditorView *view = viewForEditor(editor);
	// an IEditor doesn't have to belong to a view, it might be kept in storage by the editor model
	if (!view) {
		view = pInstance_->currentEditorView();
	}
	pInstance_->activateEditor(view, editor, flags);
}


IEditor *EditorManager::activateEditor(EditorView *view, IEditor *editor, OpenEditorFlags flags)
{
	Q_ASSERT(view);

	//    if(debugLogging)
	//        qDebug() << Q_FUNC_INFO;

	if (!editor) {
		if (!d->currentEditor_) {
			setCurrentEditor(0, (flags & IgnoreNavigationHistory));
		}
		return nullptr;
	}

	editor = placeEditor(view, editor);

	if (!(flags & DoNotChangeCurrentEditor)) 
	{
		setCurrentEditor(editor, (flags & IgnoreNavigationHistory));
		if (!(flags & DoNotMakeVisible)) 
		{
			int32_t rootIndex;
			findRoot(view, &rootIndex);

			editor->widget()->setFocus();
			ICore::raiseWindow(editor->widget());
		}
	}
	else if (!(flags & DoNotMakeVisible)) {
		view->setCurrentEditor(editor);
	}
	return editor;
}


IEditor *EditorManager::placeEditor(EditorView *view, IEditor *editor)
{
	Q_ASSERT(view && editor);

	if (view->hasEditor(editor)) {
		return editor;
	}
	if (IEditor *e = view->editorForAssetEntry(editor->assetEntry())) {
		return e;
	}

	// try duplication or pull editor over to new view
	bool duplicateSupported = editor->duplicateSupported();
	if (EditorView *sourceView = viewForEditor(editor)) 
	{
		if (editor != sourceView->currentEditor() || !duplicateSupported) 
		{
			// pull the IEditor over to the new view
			sourceView->removeEditor(editor);
			view->addEditor(editor);
			view->setCurrentEditor(editor);
			if (!sourceView->currentEditor()) {
				X_ASSERT_NOT_IMPLEMENTED();
				//        EditorView *replacementView = 0;
				//        if (IEditor *replacement = pickUnusedEditor(&replacementView)) {
				//            if (replacementView)
				//               replacementView->removeEditor(replacement);
				//            sourceView->addEditor(replacement);
				//            sourceView->setCurrentEditor(replacement);
				//        }
			}
			return editor;
		}
		else if (duplicateSupported) {
			qDebug() << "placeEditor:: duplicating";
			editor = duplicateEditor(editor);
			Q_ASSERT(editor);
		}
	}
	view->addEditor(editor);
	return editor;
}



EditorView *EditorManager::viewForEditor(IEditor *editor)
{
	QWidget *w = editor->widget();
	while (w) 
	{
		w = w->parentWidget();
		if (EditorView *view = qobject_cast<EditorView *>(w)) {
			return view;
		}
	}
	return nullptr;
}


void EditorManager::setCurrentEditor(IEditor *editor, bool ignoreNavigationHistory)
{
	Q_UNUSED(ignoreNavigationHistory);

	if (editor) {
		setCurrentView(nullptr);
	}

	if (d->currentEditor_ == editor) {
		return;
	}

	d->currentEditor_ = editor;
	if (editor) {
		if (EditorView *view = viewForEditor(editor)) {
			view->setCurrentEditor(editor);
		}
	}

	updateActions();
	updateWindowTitle();
	emit pInstance_->currentEditorChanged(editor);
}


void EditorManager::setCurrentView(EditorView *view)
{
	if (view == d->currentView_) {
		return;
	}

	EditorView *old = d->currentView_;
	d->currentView_ = view;

	if (old) {
		old->update();
	}
	if (view) {
		view->update();
	}

	if (view && !view->currentEditor()) {
		view->setFocus();
		ICore::raiseWindow(view);
	}
}


IEditor* EditorManager::createEditor(const Id& editorId, const QString& assetName)
{
	if (debugLogging) {
		qDebug() << Q_FUNC_INFO << editorId.name() << assetName;
	}

	EditorFactoryList factories;

	if (!editorId.isValid()) {
		X_ERROR("Editor", "Can't create editor for invalid editorId");
		return nullptr;
	}

	if (IEditorFactory* factory = findById<IEditorFactory>(editorId)) {
		factories.push_back(factory);
	}

	if (factories.empty())
	{
		qWarning("%s: unable to find an editor factory for the file '%s', editor Id '%s'.",
			Q_FUNC_INFO, assetName.toUtf8().constData(), editorId.name().constData());
		return nullptr;
	}

	IEditor* pEditor = factories.front()->createEditor();

	if (pEditor) {
		connect(pEditor->assetEntry(), &IAssetEntry::changed, pInstance_, &EditorManager::handleAssetEntryStateChange);

		emit pInstance_->editorCreated(pEditor, assetName);
	}

	return pEditor;
}


void EditorManager::rootDestroyed(QObject *root)
{
	QWidget *activeWin = qApp->activeWindow();
	SplitterOrView *newActiveRoot = 0;
	for (int32_t i = 0; i < d->root_.size(); ++i) {
		SplitterOrView *r = d->root_.at(i);
		if (r == root) {
			d->root_.removeAt(i);
			IContext *context = d->rootContext_.takeAt(i);
			ICore::removeContextObject(context);
			delete context;
			--i; // we removed the current one
		}
		else if (r->window() == activeWin) {
			newActiveRoot = r;
		}
	}
	// check if the destroyed root had the current view or current editor
	if (d->currentEditor_ || (d->currentView_ && d->currentView_->parentSplitterOrView() != root)) {
		return;
	}
	// we need to set a new current editor or view
	if (!newActiveRoot) {
		// some window managers behave weird and don't activate another window
		// or there might be a LadyBug toplevel activated that doesn't have editor windows
		newActiveRoot = d->root_.first();
	}

	// check if the focusWidget points to some view
	SplitterOrView *focusSplitterOrView = 0;
	QWidget *candidate = newActiveRoot->focusWidget();
	while (candidate && candidate != newActiveRoot) {
		if ((focusSplitterOrView = qobject_cast<SplitterOrView *>(candidate))) {
			break;
		}
		candidate = candidate->parentWidget();
	}
	// focusWidget might have been 0
	if (!focusSplitterOrView) {
		focusSplitterOrView = newActiveRoot->findFirstView()->parentSplitterOrView();
	}

	BUG_ASSERT(focusSplitterOrView, focusSplitterOrView = newActiveRoot);
	EditorView *focusView = focusSplitterOrView->findFirstView(); // can be just focusSplitterOrView
	BUG_ASSERT(focusView, focusView = newActiveRoot->findFirstView());
	BUG_ASSERT(focusView, return);

	if (focusView->currentEditor()) {
		setCurrentEditor(focusView->currentEditor());
	}
	else {
		setCurrentView(focusView);
	}
}



void EditorManager::closeEditor(IEditor* pEditor, bool askAboutModifiedEditors)
{ 
	if (!pEditor) {
		return;
	}
	closeEditors(QList<IEditor *>() << pEditor, askAboutModifiedEditors);
}

//void EditorManager::closeEditor(DocumentModel::Entry *entry)
//{
//	if (!entry)
//		return;
//	if (entry->document)
//		closeEditors(d->pAssetEntryModel_->editorsForDocument(entry->document));
//	else
//		d->pAssetEntryModel_->removeEntry(entry);
//}


bool EditorManager::closeEditors(const QList<IEditor*> &editorsToClose, bool askAboutModifiedEditors)
{
	Q_UNUSED(askAboutModifiedEditors);

	if (editorsToClose.isEmpty()) {
		return true;
	}

	bool closingFailed = false;
	QSet<IEditor*> acceptedEditors;
	QSet<IAssetEntry*> acceptedAssetEntry;
	//ask all core listeners to check whether the editor can be closed
	const QList<ICoreListener*> listeners = ICore::getCoreListners();

	foreach(IEditor *editor, editorsToClose) {
		bool editorAccepted = true;
		foreach(ICoreListener *listener, listeners) {
			if (!listener->editorAboutToClose(editor)) {
				editorAccepted = false;
				closingFailed = true;
				break;
			}
		}
		if (editorAccepted) {
			acceptedEditors += editor;

			QList<IEditor*> goats;
			goats = d->pAssetEntryModel_->editorsForAssetEntry(editor->assetEntry());
			goats.removeAll(editor);

			// i only want to kill the pAssetEntry if it's the last view.
			if (goats.size() == 0) {
				acceptedAssetEntry.insert(editor->assetEntry());
				qDebug() << "removing pAssetEntry: " << editor->assetEntry()->displayName();
			}
			else
			{
				X_ASSERT_NOT_IMPLEMENTED();
			//	IEditor* ed = goats.front();
				// make sure the pAssetEntry has a valid widget.
				//if (BaseTextDocument* doc = qobject_cast<BaseTextDocument*>(ed->assetEntry()))
				//{
				//	doc->SetWidget(ed->widget());
				//}
			}
		}
	}



	if (acceptedEditors.isEmpty()) {
		return false;
	}

	//ask whether to save modified files
	if (askAboutModifiedEditors)
	{
		bool cancelled = false;
		QList<IAssetEntry *> list;
		AssetEntryManager::saveModifiedAssetEntrys(acceptedAssetEntry.toList(), QString(), &cancelled, QString(), 0, &list);
		if (cancelled)
			return false;
		if (!list.isEmpty()) {
			closingFailed = true;
			acceptedAssetEntry.subtract(list.toSet());
			QSet<IEditor*> skipSet = d->pAssetEntryModel_->editorsForAssetEntrys(list).toSet();
			acceptedEditors = acceptedEditors.subtract(skipSet);
		}
	}
	if (acceptedEditors.isEmpty()) {
		return false;
	}

	QList<EditorView*> closedViews;


	// remove the editors
	foreach(IEditor *editor, acceptedEditors) {
		emit pInstance_->editorAboutToClose(editor);

		removeEditor(editor);
		if (EditorView *view = viewForEditor(editor)) 
		{
			if (editor == view->currentEditor()) {
				closedViews += view;
			}
			if (d->currentEditor_ == editor) {
				// avoid having a current editor without view
				setCurrentView(view);
				setCurrentEditor(0);
			}
			view->removeEditor(editor);
		}
	}

	foreach(EditorView *view, closedViews)
	{
		int32_t editors = view->editorCount();

		qDebug() << "editors left in view: " << editors;

		if (editors == 0)
		{
			SplitterOrView *splitter = view->parentSplitterOrView();

			BUG_CHECK(splitter);

			// if the parent is a custom window we just close the window.
			// which causes it to be remvoed from the root.
			if (BaseWindow* window = qobject_cast<BaseWindow*>(splitter->parent()))
			{
				qDebug() << "closing editor window";
				window->close();
			}
			// if the view is empty and is not the root view
			// we want to remove the split.
			else
			{
				SplitterOrView* parent = splitter->findParentSplitter();
				if (parent && parent->isSplitter()) {
					parent->unsplit(splitter);
				}
			}
		}
	}

	/*
	bool currentViewHandled = false;
	foreach (EditorView *view, closedViews)
	{
	OpenEditorFlags flags;
	if (view == currentView)
	currentViewHandled = true;
	else
	flags = OpenEditorFlags(DoNotChangeCurrentEditor);
	IEditor *newCurrent = view->currentEditor();

	if (!newCurrent)
	newCurrent = pickUnusedEditor();
	if (newCurrent) {
	activateEditor(view, newCurrent, flags);
	}
	else
	{
	DocumentModel::Entry *entry = d->pAssetEntryModel_->firstRestoredDocument();
	if (entry)
	{
	activateEditorForEntry(view, entry, flags);
	} else
	{
	// no "restored" ones, so any entry left should have a document
	const QList<DocumentModel::Entry *> documents = d->pAssetEntryModel_->documents();
	if (!documents.isEmpty()) {
	IDocument *document = documents.last()->document;
	if (document)
	activateEditorForDocument(view, document, flags);
	}
	}
	}
	}*/


	emit pInstance_->editorsClosed(acceptedEditors.toList());

	foreach(IEditor *editor, acceptedEditors) {
		delete editor;
	}

	if (!currentEditor()) {
		emit pInstance_->currentEditorChanged(0);
		updateActions();
		updateWindowTitle();
	}


	return !closingFailed;
}


IEditor* EditorManager::pickUnusedEditor(EditorView **foundView)
{
	foreach(IEditor *editor, d->pAssetEntryModel_->editorsForAssetEntrys(d->pAssetEntryModel_->openedAssetEntrys())) 
	{
		EditorView *view = viewForEditor(editor);
		if (!view || view->currentEditor() != editor) {
			if (foundView) {
				*foundView = view;
			}
			return editor;
		}
	}
	return nullptr;
}


void EditorManager::activateEditorForEntry(AssetEntryModel::Entry *entry, OpenEditorFlags flags)
{
	activateEditorForEntry(currentEditorView(), entry, flags);
}


void EditorManager::activateEditorForEntry(EditorView *view, AssetEntryModel::Entry *entry, OpenEditorFlags flags)
{
	BUG_ASSERT(view, return);
	if (!entry) { // no pAssetEntry
		view->setCurrentEditor(0);
		setCurrentView(view);
		setCurrentEditor(0);
		return;
	}
	IAssetEntry* pAssetEntry = entry->pAssetEntry_;
	if (pAssetEntry) {
		activateEditorForAssetEntry(view, pAssetEntry, flags);
		return;
	}

	if (!openEditor(view, entry->assetName(), entry->type(), entry->id(), flags)) {
		d->pAssetEntryModel_->removeEntry(entry);
	}
}


IEditor *EditorManager::activateEditorForAssetEntry(IAssetEntry* pAssetEntry, OpenEditorFlags flags)
{
	return activateEditorForAssetEntry(currentEditorView(), pAssetEntry, flags);
}

IEditor *EditorManager::activateEditorForAssetEntry(EditorView *view, IAssetEntry* pAssetEntry, OpenEditorFlags flags)
{
	Q_ASSERT(view);
	IEditor *editor = view->editorForAssetEntry(pAssetEntry);
	if (!editor) 
	{
		const QList<IEditor*> editors = d->pAssetEntryModel_->editorsForAssetEntry(pAssetEntry);
		if (editors.isEmpty()) {
			return nullptr;
		}
		editor = editors.first();
	}
	return activateEditor(view, editor, flags);
}

//DocumentModel *EditorManager::documentModel(void)
//{
//	return d->pAssetEntryModel_;
//}

bool EditorManager::closeAssetEntrys(const QList<IAssetEntry*>& assetEntrys, bool askAboutModifiedEditors)
{
	return pInstance_->closeEditors(d->pAssetEntryModel_->editorsForAssetEntrys(assetEntrys), askAboutModifiedEditors);
}


IEditor* EditorManager::duplicateEditor(IEditor *editor)
{
	if (!editor->duplicateSupported()) {
		return nullptr;
	}

	IEditor *duplicate = editor->duplicate();
	duplicate->restoreState(editor->saveState());
	emit pInstance_->editorCreated(duplicate, duplicate->assetEntry()->name());
	addEditor(duplicate);
	return duplicate;
}

qint64 EditorManager::maxTextFileSize(void)
{
	return qint64(3) << 24;
}

void EditorManager::setWindowTitleAddition(const QString& addition)
{
	d->titleAddition_ = addition;
	updateWindowTitle();
}

QString EditorManager::windowTitleAddition(void)
{
	return d->titleAddition_;
}

void EditorManager::setWindowTitleVcsTopic(const QString& topic)
{
	d->titleVcsTopic_ = topic;
	pInstance_->updateWindowTitle();
}

QString EditorManager::windowTitleVcsTopic(void)
{
	return d->titleVcsTopic_;
}

bool EditorManager::saveEditor(IEditor *editor)
{
	return saveAssetEntry(editor->assetEntry());
}

bool EditorManager::saveAssetEntry(IAssetEntry* pAssetEntryParam)
{
	IAssetEntry* pAssetEntry = pAssetEntryParam;
	if (!pAssetEntry && currentAssetEntry()) {
		pAssetEntry = currentAssetEntry();
	}
	if (!pAssetEntry) {
		return false;
	}

//	pAssetEntry->checkPermissions();

	const bool success = AssetEntryManager::saveAssetEntry(pAssetEntry);

	if (success) {
		addAssetEntryToRecentFiles(pAssetEntry);
	}
	else {
		const auto name = pAssetEntry->name().toStdString();
		X_WARNING("EditorMan", "Failed to save asset entry: \"%s\"", name.c_str());
	}

	return success;
}


bool EditorManager::saveAssetEntryAs(IAssetEntry* pAssetEntryParam)
{
	X_ASSERT_NOT_IMPLEMENTED();
	X_UNUSED(pAssetEntryParam);
	return false;
}


void EditorManager::handleAssetEntryStateChange(void)
{
	updateActions();

	IAssetEntry* pAssetEntry = qobject_cast<IAssetEntry*>(sender());

	if (EditorManager::currentAssetEntry() == pAssetEntry) {
		emit pInstance_->currentAssetEntryStateChanged();
	}
}


/* Adds the file name to the recent files if there is at least one non-temporary editor for it */
void EditorManager::addAssetEntryToRecentFiles(IAssetEntry* pAssetEntry)
{
	if (pAssetEntry->isTemporary()) {
		return;
	}

	AssetEntryModel::Entry* pEntry = d->pAssetEntryModel_->entryForAssetEntry(pAssetEntry);
	if (!pEntry) {
		return;
	}

	AssetEntryManager::addToRecentFiles(pAssetEntry->name(), pAssetEntry->type(), pEntry->id());
}

void EditorManager::autoSave(void)
{
	// QStringList errors;
	// save the open AssetEntrys.
	QStringList errors;

	// FIXME: the saving should be staggered
	foreach(IAssetEntry* pAssetEntry, d->pAssetEntryModel_->openedAssetEntrys())
	{
		if (!pAssetEntry->isModified() || !pAssetEntry->shouldAutoSave()) {
			continue;
		}
		if (pAssetEntry->name().isEmpty()) { // FIXME: save them to a dedicated directory
			continue;
		}

		QString errorString;
		if (!pAssetEntry->autoSave(&errorString)) {
			errors << errorString;
		}
	}

	if (!errors.isEmpty()) {
		QMessageBox::critical(ICore::mainWindow(), tr("File Error"), errors.join(QLatin1String("\n")));
	}

	// Also save settings while accessing the disk anyway:
	//ICore::saveSettings();
}

void EditorManager::updateWindowTitle(void)
{
	QString windowTitle = tr("Editor");
	const QString dashSep = QLatin1String(" - ");
	QString vcsTopic;
	IAssetEntry* pAssetEntry = currentAssetEntry();

	//    EditorManagerPrivate* temp = d;

	if (!d->titleVcsTopic_.isEmpty()) {
		vcsTopic = QLatin1String(" [") + d->titleVcsTopic_ + QLatin1Char(']');
	}

	if (!d->titleAddition_.isEmpty()) {
		windowTitle.prepend(dashSep);
		if (!pAssetEntry) {
			windowTitle.prepend(vcsTopic);
		}
		windowTitle.prepend(d->titleAddition_);
	}

	if (pAssetEntry) {
		const QString assetEntryName = pAssetEntry->displayName();
		if (!assetEntryName.isEmpty()) {
			windowTitle.prepend(assetEntryName + vcsTopic + dashSep);
		}

		QString assetName = pAssetEntry->name();
		if (!assetName.isEmpty()) {
			ICore::mainWindow()->setWindowFilePath(assetName);
		}
	}
	else {
		ICore::mainWindow()->setWindowFilePath(QString());
	}
	ICore::mainWindow()->setWindowTitle(windowTitle);
}



void EditorManager::closeView(EditorView* pView)
{
	if (!pView) {
		return;
	}

	emptyView(pView);

	SplitterOrView *splitterOrView = pView->parentSplitterOrView();
	Q_ASSERT(splitterOrView);
	Q_ASSERT(splitterOrView->view() == pView);
	SplitterOrView *splitter = splitterOrView->findParentSplitter();
	Q_ASSERT(splitterOrView->hasEditors() == false);
	splitterOrView->hide();

	// check if it was a custom window.
	BaseWindow* window = qobject_cast<BaseWindow*>(splitterOrView->parent());

	delete splitterOrView;

	if (!splitter)
	{
		if (window)
		{
			window->close();
		}
		return;
	}

	splitter->unsplit();

	EditorView *newCurrent = splitter->findFirstView();
	if (newCurrent) {
		if (IEditor *e = newCurrent->currentEditor()) {
			activateEditor(newCurrent, e);
		}
		else {
			setCurrentView(newCurrent);
		}
	}
}


bool EditorManager::closeAllEditors(bool askAboutModifiedEditors)
{
	if (closeAssetEntrys(d->pAssetEntryModel_->openedAssetEntrys(), askAboutModifiedEditors)) {
		return true;
	}
	return false;
}


void EditorManager::closeOtherEditors(IAssetEntry* pAssetEntry)
{
	QList<IAssetEntry*> assetEntrysToClose = d->pAssetEntryModel_->openedAssetEntrys();
	assetEntrysToClose.removeAll(pAssetEntry);
	closeAssetEntrys(assetEntrysToClose, true);
}


void EditorManager::emptyView(EditorView *view)
{
	if (!view) {
		return;
	}

	EditorManager* pInstance = pInstance_;
	if (!pInstance)
	{
		qDebug() << "emptyview called after editormanger closed";
		return;
	}

	QList<IEditor*> editors = view->editors();
	foreach(IEditor *editor, editors)
	{
		if (d->pAssetEntryModel_->editorsForAssetEntry(editor->assetEntry()).size() == 1)
		{
			// it's the only editor for that file
			// so we need to keep it around (--> in the editor model)
			if (currentEditor() == editor) {
				// we don't want a current editor that is not open in a view
				setCurrentView(view);
				setCurrentEditor(nullptr);
			}
			editors.removeAll(editor);
			view->removeEditor(editor);
			continue; // don't close the editor
		}
		emit pInstance_->editorAboutToClose(editor);
		removeEditor(editor);
		view->removeEditor(editor);
	}

	if (!editors.isEmpty()) {
		emit pInstance_->editorsClosed(editors);
		foreach(IEditor *editor, editors) {
			delete editor;
		}
	}
}


void EditorManager::updateActions(void)
{
	IAssetEntry* pCurAssEntry = currentAssetEntry();
	int32_t openedCount = d->pAssetEntryModel_->assetEntryCount();

	foreach(SplitterOrView *root, d->root_) {
		setCloseSplitEnabled(root, root->isSplitter());
	}

	QString quotedName;
	if (pCurAssEntry) {
		quotedName = QLatin1Char('"') + pCurAssEntry->displayName() + QLatin1Char('"');
	}
	setupSaveActions(pCurAssEntry, d->saveAction_, nullptr);


	d->closeCurrentEditorAction_->setEnabled(pCurAssEntry);
	d->closeCurrentEditorAction_->setText(tr("Close %1").arg(quotedName));
	d->closeAllEditorsAction_->setEnabled(openedCount > 0);
	d->closeOtherEditorsAction_->setEnabled(openedCount > 1);
	d->closeOtherEditorsAction_->setText(tr("Close All But This"));

}

void EditorManager::setCloseSplitEnabled(SplitterOrView *splitterOrView, bool enable)
{
	//  this function currently dose jack shit lol.
	//    if (splitterOrView->isView())
	//        splitterOrView->view()->setCloseSplitEnabled(enable);

	QSplitter *splitter = splitterOrView->splitter();
	if (splitter) {
		for (int32_t i = 0; i < splitter->count(); ++i) {
			if (SplitterOrView *subSplitterOrView = qobject_cast<SplitterOrView*>(splitter->widget(i))) {
				setCloseSplitEnabled(subSplitterOrView, enable);
			}
		}
	}
}


void EditorManager::setupSaveActions(IAssetEntry* pAssetEntry, QAction *saveAction, QAction *revertToSavedAction)
{
//	if (debugLogging) {
//		qDebug() << Q_FUNC_INFO;
//	}

	saveAction->setEnabled(pAssetEntry != nullptr && pAssetEntry->isModified());
	if (revertToSavedAction) {
		revertToSavedAction->setEnabled(pAssetEntry != nullptr && !pAssetEntry->name().isEmpty());
	}

	const QString assetEntryName = pAssetEntry ? pAssetEntry->displayName() : QString();
	QString quotedName;

	if (!assetEntryName.isEmpty()) {
		quotedName = QLatin1Char('"') + assetEntryName + QLatin1Char('"');
		saveAction->setText(tr("&Save %1").arg(quotedName));
	}
}



static void assignAction(QAction *self, QAction *other)
{
	self->setText(other->text());
	self->setIcon(other->icon());
	self->setShortcut(other->shortcut());
	self->setEnabled(other->isEnabled());
	self->setIconVisibleInMenu(other->isIconVisibleInMenu());
}

void EditorManager::addSaveAndCloseEditorActions(QMenu *contextMenu, IEditor *editor)
{
	BUG_ASSERT(contextMenu, return);
	d->contextMenuEntry_ = editor;

	assignAction(d->saveCurrentEditorContextAction_, ActionManager::command(Constants::SAVE)->action());

	IAssetEntry* pAssetEntry = editor ? editor->assetEntry() : nullptr;

	setupSaveActions(pAssetEntry,
		d->saveCurrentEditorContextAction_,
		nullptr
	);

	contextMenu->addAction(d->saveCurrentEditorContextAction_);
	contextMenu->addAction(ActionManager::command(Constants::SAVEALL)->action());

	contextMenu->addSeparator();

	d->closeCurrentEditorContextAction_->setText(pAssetEntry
		? tr("Close \"%1\"").arg(pAssetEntry->displayName())
		: tr("Close Editor"));
	d->closeOtherEditorsContextAction_->setText(tr("Close All But This"));
	d->closeCurrentEditorContextAction_->setEnabled(pAssetEntry != nullptr);
	d->closeOtherEditorsContextAction_->setEnabled(pAssetEntry != nullptr);
	d->closeAllEditorsContextAction_->setEnabled(!d->pAssetEntryModel_->assetEntrys().isEmpty());
	contextMenu->addAction(d->closeCurrentEditorContextAction_);
	contextMenu->addAction(d->closeAllEditorsContextAction_);
	contextMenu->addAction(d->closeOtherEditorsContextAction_);
}

void EditorManager::addNativeDirActions(QMenu *contextMenu, IEditor *editor)
{
	BUG_ASSERT(contextMenu, return);
	X_UNUSED(editor);
//	bool enabled = editor && !editor->assetEntry()->name().isEmpty();

//	d->openGraphicalShellAction_->setEnabled(enabled);

	contextMenu->addAction(d->copyFullPathContextAction_);
//	contextMenu->addAction(d->openGraphicalShellAction_);
}


void EditorManager::addFloatActions(QMenu *contextMenu, IEditor* pEditor)
{
	BUG_ASSERT(contextMenu, return);

	// we need to know if a window is floated.
	if (EditorView* pView = viewForEditor(pEditor)) 
	{
		SplitterOrView* pSplitter = pView->parentSplitterOrView();

		if (pSplitter) {

			int32_t rootidx = -1;
			findRoot(pView, &rootidx);

			if (rootidx != 0) {
				contextMenu->addAction(d->DockEditorMainContextAction_);
			}
			else {
				contextMenu->addAction(d->FloatEditorContextAction_);
			}

			contextMenu->addSeparator();

			if (!pSplitter->isView()) {
				return;
			}

			int32_t num_editors = pSplitter->view()->editorCount();

			// if the view is part of a splitter already we don't allow split again.
			pSplitter = pSplitter->findParentSplitter();

			if (!pSplitter) {
				// we can only move to new if move than 1 baby
				if (num_editors > 1) {
					contextMenu->addAction(d->moveToNewHozTabGroupContextAction_);
					contextMenu->addAction(d->moveToNewVerTabGroupContextAction_);
					contextMenu->addSeparator();
				}

				if (pEditor->duplicateSupported()) {
					contextMenu->addAction(d->splitHozTabGroupContextAction_);
					contextMenu->addAction(d->splitVerTabGroupContextAction_);
				}
			}

		}
	}
}



void EditorManager::saveAssetEntryFromContextMenu(void)
{
	IAssetEntry* pAssetEntry = d->contextMenuEntry_ ? d->contextMenuEntry_->assetEntry() : nullptr;
	if (pAssetEntry) {
		saveAssetEntry(pAssetEntry);
	}
}

void EditorManager::closeEditorFromContextMenu(void)
{
	IAssetEntry* pAssetEntry = d->contextMenuEntry_ ? d->contextMenuEntry_->assetEntry() : nullptr;
	if (pAssetEntry) {
		closeEditors(d->pAssetEntryModel_->editorsForAssetEntry(pAssetEntry));
	}
}

void EditorManager::closeOtherEditorsFromContextMenu(void)
{
	IAssetEntry* pAssetEntry = d->contextMenuEntry_ ? d->contextMenuEntry_->assetEntry() : nullptr;
	closeOtherEditors(pAssetEntry);
}

void EditorManager::copyFullPath(void)
{
	if (!d->contextMenuEntry_ || d->contextMenuEntry_->assetEntry()->name().isEmpty()) {
		return;
	}

	QApplication::clipboard()->setText(d->contextMenuEntry_->assetEntry()->name());
}



// =============== Split me ====================

void EditorManager::RemoveSplitIfEmpty(EditorView *view)
{
	if (view->editorCount() != 0) {
		return;
	}

	// the views view object.
	SplitterOrView* splitView = view->parentSplitterOrView();

	// the parent needs to be a split to remove the split :@)
	if (SplitterOrView* splitter = splitView->findParentSplitter())
	{
		if (splitter->isSplitter())
		{
			// remove split, passing the view we want removed.
			splitter->unsplit(splitView);
		}
	}
}

void EditorManager::split(Qt::Orientation orientation, IEditor *editor)
{
	EditorView* view = currentEditorView();

	if (editor) {
		view = viewForEditor(editor);
	}

	if (view) {
		view->parentSplitterOrView()->split(orientation);
	}

	updateActions();
}

void EditorManager::split(void)
{
	split(Qt::Vertical);
}

void EditorManager::splitSideBySide(void)
{
	split(Qt::Horizontal);
}

void EditorManager::splitNewWindow(void)
{
	splitNewWindow(currentEditorView());
}

void EditorManager::splitNewWindow(IEditor *editor)
{
	if (EditorView *view = viewForEditor(editor)) {
		splitNewWindow(view, editor);
	}
}

void EditorManager::splitNewWindow(EditorView* pView, IEditor* pEditor)
{
	BaseWindow* pNewWindow = new BaseWindow;

	pEditor = pEditor ? pEditor : pView->currentEditor();

	pView->removeEditor(pEditor);

	QSize size = pView->geometry().size();
	size += QSize(12, 40);

	RemoveSplitIfEmpty(pView);

	pNewWindow->setWindowTitle(pEditor->assetEntry()->displayName() + " - Editor");
	pNewWindow->setAttribute(Qt::WA_DeleteOnClose);
	pNewWindow->setAttribute(Qt::WA_QuitOnClose, false); // close when main window closes.
	pNewWindow->resize(size);

	SplitterOrView* splitter = new SplitterOrView(pNewWindow);

	IContext *context = new IContext;
	context->setContext(Context(Constants::C_EDITORMANAGER));
	context->setWidget(pNewWindow);
	ICore::addContextObject(context);

	d->root_.append(splitter);
	d->rootContext_.append(context);

	connect(splitter, SIGNAL(destroyed(QObject*)), pInstance_, SLOT(rootDestroyed(QObject*)));

	pNewWindow->show();
	ICore::raiseWindow(pNewWindow);

	if (pEditor) {
		pInstance_->activateEditor(splitter->view(), pEditor, IgnoreNavigationHistory);
	}
	else {
		splitter->view()->setFocus();
	}

	pInstance_->updateActions();
}


void EditorManager::removeCurrentSplit(void)
{
	EditorView *viewToClose = currentEditorView();

	BUG_ASSERT(viewToClose, return);
	BUG_ASSERT(!d->root_.contains(viewToClose->parentSplitterOrView()), return);

	closeView(viewToClose);
	updateActions();
}


void EditorManager::removeAllSplits(void)
{
	EditorView *view = currentEditorView();
	BUG_ASSERT(view, return);
	SplitterOrView *root = findRoot(view);
	BUG_ASSERT(root, return);
	root->unsplitAll();
}


void EditorManager::floatEditor(void)
{
	if (d->contextMenuEntry_) {
		splitNewWindow(d->contextMenuEntry_);
	}
}

void EditorManager::dockEditorMain(void)
{
	IEditor* editor = d->contextMenuEntry_;

	BUG_ASSERT(editor, return);

	if (EditorView* pView = viewForEditor(editor)) 
	{
		// we remove the editor and place it in main view
		// if the editor is the only one in the float we remove the float window.
		SplitterOrView* pSplitter = pView->parentSplitterOrView();

		BUG_ASSERT(pSplitter, return);

		pView->removeEditor(editor);


		int32_t count = pView->editorCount();
		qDebug() << "editor count: " << count;

		if (count == 0) {
			if (BaseWindow* window = qobject_cast<BaseWindow*>(pSplitter->parent())) {
				qDebug() << "closing editor window";
				window->close();
			}
			else {
				RemoveSplitIfEmpty(pView);
			}
		}

		// Add to the main edtior.
		SplitterOrView* pNewView = d->root_.at(0);

		// if main is split we need to find the first non split and slap it there.
		if (pNewView->isSplitter()) {
			pNewView = pNewView->findFirstView()->parentSplitterOrView();
		}

		pInstance_->activateEditor(pNewView->view(), editor, IgnoreNavigationHistory);

		pNewView->view()->setCurrentEditor(editor);
	}
}

void EditorManager::newHozTabGroup(void)
{
	// qt has lost the plot, Vertical is (real)Horizontal jews!
	if (d->contextMenuEntry_) {
		split(Qt::Horizontal, d->contextMenuEntry_);
	}
}

void EditorManager::newVerTabGroup(void)
{
	if (d->contextMenuEntry_) {
		split(Qt::Vertical, d->contextMenuEntry_);
	}
}


// =============== Move me ====================

void EditorManager::moveNewHozTabGroup(void)
{
	if (d->contextMenuEntry_) {
		moveEditor(Qt::Horizontal, d->contextMenuEntry_);
	}
}

void EditorManager::moveNewVerTabGroup(void)
{
	if (d->contextMenuEntry_) {
		moveEditor(Qt::Vertical, d->contextMenuEntry_);
	}
}


void EditorManager::moveEditor(Qt::Orientation orientation, IEditor *editor, Direction side)
{
	// we want to move the editor to a new group.
	// the orientation is the direction of the new spit.
	// this dose require there to be atleast 2 editors in the EditorView
	BUG_ASSERT(editor, return);

	EditorView *view = viewForEditor(editor);

	BUG_CHECK(view);

	if (view)
	{
		if (view->editorCount() > 1)
		{
			// ok we want to remove the view.
			// then do a split and place it in new view.
			SplitterOrView* splitter = view->parentSplitterOrView();

			BUG_ASSERT(splitter, return);

			splitter->moveEditor(orientation, editor, side);
		}
		else
		{
			qDebug() << "need alteast 2 editors in tabview to perform a move.";
		}
	}
}


// =============== UnDock me ====================


SplitterOrView* EditorManager::undockEditor(IEditor* editor, QPoint& pos)
{
	BUG_ASSERT(editor, return 0);

	// slap a chicken.
	EditorView* pView = viewForEditor(editor);

	BUG_ASSERT(pView, return 0);

	pView->removeEditor(editor);

	QSize size = pView->size();
	size += QSize(0, 40);


	if (pView->editorCount() == 0)
	{
		SplitterOrView* parent = pView->parentSplitterOrView();
		if (BaseWindow* window = qobject_cast<BaseWindow*>(parent->parent())) {
			delete window;
		}
		else {
			RemoveSplitIfEmpty(pView);
		}
	}

	// it has no context or anything yet.
	// since this is only for dragging around.
	SplitterOrView *splitter = new SplitterOrView(editor);
	splitter->SetDragMode(pos);
	splitter->resize(size);
	splitter->move(QCursor::pos() - pos);
	splitter->show();
	splitter->grabMouse(Qt::ArrowCursor);

	return splitter;
}


void EditorManager::splitDragEndWindow(BaseWindow* window, SplitterOrView *splitter)
{
	// hide the overlay.
	d->overlay_->hide();

	Overlay::CurButton button = d->overlay_->getActiveButton();
	EditorView* target = d->dropTargetView_;


	if (button != Overlay::CurButton::Invalid && target)
	{
		// if not center we want to keep the layout.
		// all we need to do really is take the splitter
		// and place it in the layout of the target
		// the location is dependant on selected button.
		if (button != Overlay::CurButton::Center)
		{
			SplitterOrView *target_split = target->parentSplitterOrView();

			if (target_split)
			{
				switch (button)
				{
				case Overlay::CurButton::Right:
					target_split->addSplitter(splitter, Qt::Horizontal, Direction::Right);
					break;
				case Overlay::CurButton::Left:
					target_split->addSplitter(splitter, Qt::Horizontal, Direction::Left);
					break;

				case Overlay::CurButton::Top:
					target_split->addSplitter(splitter, Qt::Vertical, Direction::Top);
					break;
				case Overlay::CurButton::Bottom:
					target_split->addSplitter(splitter, Qt::Vertical, Direction::Bottom);
					break;
				}

			}
		}
		// if it's center we want to add all the views as tabs o.o !
		else
		{
			if (splitter->isView())
			{
				AddEditorsToView(splitter->view(), target, splitter->editors());
			}
			else
			{
				// some fuck nut be dropping a window that's got splits in it yo !
				AddEditorsToView(splitter, target);
			}
		}

		//   m_instance->updateActions();
		//   delete oldSplitter;
		//   delete window;
		window->deleteLater();
	}
}

void EditorManager::splitDragEnd(SplitterOrView* pSplitter)
{
	BUG_ASSERT(pSplitter->isDrag(), return);
	BUG_ASSERT(pSplitter->isView(), return);

	// hide the overlay.
	d->overlay_->hide();

	Overlay::CurButton button = d->overlay_->getActiveButton();


	SplitterOrView *oldSplitter = pSplitter;
	IEditor* editor;
	EditorView *view;
	QSize size;
	QPoint pos;

	// we should also have only one editor.
	editor = pSplitter->editor();
	view = pSplitter->view();

	pos = pSplitter->pos();
	pos -= QPoint(5, 32);
	size = pSplitter->size();

	view->removeEditor(editor);

	if (button == Overlay::CurButton::Invalid || !d->dropTargetView_)
	{
		// ok we add the pickle to a new window.
		BaseWindow* pNewWindow = new BaseWindow;

		pNewWindow->setWindowTitle(editor->assetEntry()->displayName() + " - Editor");
		pNewWindow->setAttribute(Qt::WA_DeleteOnClose);
		pNewWindow->setAttribute(Qt::WA_QuitOnClose, false); // close when main window closes.
		pNewWindow->resize(size);

		pSplitter = new SplitterOrView(pNewWindow);

		IContext *context = new IContext;
		context->setContext(Context(Constants::C_EDITORMANAGER));
		context->setWidget(pNewWindow);
		ICore::addContextObject(context);

		d->root_.append(pSplitter);
		d->rootContext_.append(context);

		connect(pSplitter, SIGNAL(destroyed(QObject*)), pInstance_, SLOT(rootDestroyed(QObject*)));

		pNewWindow->show();
		pNewWindow->move(pos);

		pInstance_->activateEditor(pSplitter->view(), editor, IgnoreNavigationHistory);
	}
	else
	{
		EditorView* target = d->dropTargetView_;

		// ok so we want to add the editor to this view
		// then move it to a new split basically.
		pInstance_->activateEditor(target, editor, IgnoreNavigationHistory);


		// check for hte diffrent types
		switch (button)
		{
		case Overlay::CurButton::Right:
			moveEditor(Qt::Horizontal, editor, Direction::Right);
			break;
		case Overlay::CurButton::Left:
			moveEditor(Qt::Horizontal, editor, Direction::Left);
			break;

		case Overlay::CurButton::Top:
			moveEditor(Qt::Vertical, editor, Direction::Top);
			break;
		case Overlay::CurButton::Bottom:
			moveEditor(Qt::Vertical, editor, Direction::Bottom);
			break;

		default:
			target->setCurrentEditor(editor);
			break;
		}
	}


	pInstance_->updateActions();
	delete oldSplitter;
}

void EditorManager::AddEditorsToView(EditorView *source, EditorView* target, QList<IEditor*>& editors)
{
	foreach(IEditor* editor, editors)
	{
		source->removeEditor(editor);

		OpenEditorFlags flags = IgnoreNavigationHistory | DoNotChangeCurrentEditor;

		pInstance_->activateEditor(target, editor, flags);

		if (editor == editors.last()) {
			target->setCurrentEditor(editor);
		}
	}
}


void EditorManager::AddEditorsToView(SplitterOrView* splitter, EditorView* target)
{
	if (!splitter->isSplitter()) {
		return; // could redirect it to above function.
	}

	QSplitter* split = splitter->splitter();

	for (int32_t i = 0; i< split->count(); i++) 
	{
		SplitterOrView* s = qobject_cast<SplitterOrView*>(split->widget(i));

		if (s) {
			if (s->isView()) {
				AddEditorsToView(s->view(), target, s->editors());
			}
			else if (s->isSplitter()) {
				AddEditorsToView(s, target);
			}
		}
	}
}

void EditorManager::floatDockCheck(SplitterOrView *splitter, const QPoint& pos)
{
	// ok get all the editor views that are visable.
	QList<EditorView*> views = visibleViews();

	//    qDebug() << "Num visabel views: " << views.count();
	d->dropTargetView_ = nullptr;

	if (!views.empty())
	{
		bool fnd = false;
		QRect rect;

		// lets just make sure a view never shows drop overlay on itself.
		if (splitter->isView())
			views.removeOne(splitter->view());

		foreach(EditorView* view, views)
		{
			// we need to check if we are over that editor.
			if (view->rect().contains(view->mapFromGlobal(pos)))
			{
				fnd = true;
				rect = view->rect();
				rect.moveTopLeft(view->mapToGlobal(view->rect().topLeft()));

				d->dropTargetView_ = view;
				break;
			}
		}

		ShowDropOverlay(fnd, rect);
	}
}

void EditorManager::ShowDropOverlay(bool show, const QRect& rect)
{
	if (show) {
		d->overlay_->updatePos(rect);
	}

	if (d->overlay_)
	{
		if (show && !d->overlay_->isVisible()) {
			d->overlay_->show();
		}
		else if (!show && d->overlay_->isVisible()) {
			d->overlay_->hide();
		}
	}
}


X_NAMESPACE_END