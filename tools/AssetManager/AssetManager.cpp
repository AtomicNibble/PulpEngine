#include "assetmanager.h"
#include "assetdbwidget.h"
#include "assetdbexplorer.h"
#include "ActionManager.h"
#include "EditorManager.h"
#include "ActionContainer.h"
#include "VersionDialog.h"
#include "AddAssetDialog.h"
#include "StatusBar.h"
#include "OutputPaneManagerWidget.h"
#include "OutputWindowWidget.h"
#include "OutputWindowPane.h"

#include "AssetEntryManager.h"
#include "AssetPropertyEditorFactory.h"
#include "AssetScript.h"
#include "Command.h"

#include "IEditor.h"
#include "IAssetEntry.h"

#include <../AssetDB/AssetDB.h>

#include <qfilesystemwatcher.h>

X_NAMESPACE_BEGIN(assman)

AssetManager::AssetManager(QWidget* pParent) :
	BaseWindow(pParent),
	pVersionDialog_(nullptr),
	pLayout_(nullptr),
	pCoreImpl_(nullptr),
	pWatcher_(nullptr),
	pActionManager_(nullptr),
	pEditorManager_(nullptr),
	pDb_(nullptr),
	pAssetDbexplorer_(nullptr),
	additionalContexts_(Constants::C_GLOBAL) // always have global contex
{
	pStatusBar_ = new MyStatusBar();
	pWatcher_ = new QFileSystemWatcher(this);
	pCoreImpl_ = new ICore(this);
	pActionManager_ = new ActionManager(this);
	pAssetEntryManager_ = new AssetEntryManager(this);

	// Logging.
	pOutputWindow_ = new OutputWindow(Context(Constants::C_GENERAL_OUTPUT_PANE));
	pOutputWindow_->setWordWrapEnabled(false);
	pLoggerPolicy_ = new OutputWindowWrtiePolicy(
		OutputWindowWrtiePolicy::FilterPolicy(),
		OutputWindowWrtiePolicy::FormatPolicy(),
		OutputWindowWrtiePolicy::WritePolicy(pOutputWindow_)
	);
	gEnv->pLog->AddLogger(pLoggerPolicy_);

	OutputPaneManager::create();

	pLayout_ = new QGridLayout();
	pDockArea_ = new QMainWindow();

	{
		pDb_ = new assetDb::AssetDB();
		if (!pDb_->OpenDB()) {
			QMessageBox::critical(this, tr("Error"), "Failed to open AssetDB");
		}
	}

	connect(pWatcher_, SIGNAL(fileChanged(const QString &)),
		this, SLOT(fileChanged(const QString &)));

	pWatcher_->addPath("style\\style.qss");

	// ----------------------------------

	createActions();
	createMenus();
	createStatusBar();

	// ----------------------------------

	pAssetScripts_ = new AssetPropsScriptManager();
	if (!pAssetScripts_->init()) {
		QMessageBox::critical(this, tr("Error"), "Failed to init AssetScript manager");
	}

	// needs to be done after menu's created
	pEditorManager_ = new EditorManager(this);
	pEditorManager_->init();

	pEditorManager_->AddFactory(new AssetPropertyEditorFactory(this));

	{
		pAssetDbexplorer_ = new AssetExplorer::AssetExplorer(*pDb_);
		if (!pAssetDbexplorer_->init()) {
			QMessageBox::critical(this, tr("Error"), "Failed to init AssetExpolrer");
		}
		pAssetDbexplorer_->loadMods();
	}

	connect(QApplication::instance(), SIGNAL(focusChanged(QWidget*, QWidget*)),
		this, SLOT(updateFocusWidget(QWidget*, QWidget*)));


	createDockWindows();

	OutputPaneManager::instance()->addPane(new OutputWindowPane(pOutputWindow_));
	OutputPaneManager::instance()->init();

	pDockArea_->setCentralWidget(pEditorManager_);
	pLayout_->addItem(new QSpacerItem(2, 2), 0, 0, 1, 1); // left
	pLayout_->addWidget(pDockArea_, 0, 1, 1, 1); // center`
	pLayout_->addItem(new QSpacerItem(2, 2), 0, 2, 1, 1); // right
	setStatusBar(pStatusBar_);
	setCentralWidget(pLayout_);
	setMinimumSize(600, 800);


	EditorManager::openEditor("test", Constants::ASSETPROP_EDITOR_ID);
	EditorManager::openEditor("test1", Constants::ASSETPROP_EDITOR_ID);
	EditorManager::openEditor("test2", Constants::ASSETPROP_EDITOR_ID);

}

AssetManager::~AssetManager()
{
	if (pLoggerPolicy_) {
		gEnv->pLog->RemoveLogger(pLoggerPolicy_);
		delete pLoggerPolicy_;
	}

	OutputPaneManager::destroy();

	if (pAssetScripts_) {
		delete pAssetScripts_;
	}

	if (pDb_) {
		pDb_->CloseDB();
		delete pDb_;
	}

	if (pAssetEntryManager_) {
		delete pAssetEntryManager_;
	}

	if (pAssetDbexplorer_) {
		delete pAssetDbexplorer_;
	}

	if (pCoreImpl_) {
		delete pCoreImpl_;
	}

	if (pActionManager_) {
		delete pActionManager_;
	}

	if (pEditorManager_) {
		delete pEditorManager_;
	}

}



IContext* AssetManager::currentContextObject(void) const
{
	return activeContext_.isEmpty() ? nullptr : activeContext_.first();
}

IContext* AssetManager::contextObject(QWidget* pWidget)
{
	return contextWidgets_.value(pWidget);
}

void AssetManager::addContextObject(IContext* pContex)
{
	if (!pContex) {
		return;
	}

	QWidget* pWidget = pContex->widget();
	if (contextWidgets_.contains(pWidget)) {
		return;
	}

	contextWidgets_.insert(pWidget, pContex);
}

void AssetManager::removeContextObject(IContext* pContex)
{
	if (!pContex) {
		return;
	}

	QWidget* pWidget = pContex->widget();
	if (!contextWidgets_.contains(pWidget)) {
		return;
	}

	contextWidgets_.remove(pWidget);

	if (activeContext_.removeAll(pContex) > 0) {
		updateContextObject(activeContext_);
	}
}

void AssetManager::updateFocusWidget(QWidget* old, QWidget* now)
{
	X_UNUSED(old);

	// Prevent changing the context object just because the menu or a menu item is activated
	if (qobject_cast<QMenuBar*>(now) || qobject_cast<QMenu*>(now)) {
		return;
	}

#if X_DEBUG
	if (debugLogging && now && now->metaObject()) {
		qDebug() << "Name: " << now->metaObject()->className();
	}
#endif // ¬X_DEBUG

	QList<IContext*> newContext;
	if (QWidget* pWidget = qApp->focusWidget())
	{
		IContext* pContext = nullptr;
		while (pWidget)
		{
			pContext = contextWidgets_.value(pWidget);
			if (pContext) {
				newContext.append(pContext);
			}
			pWidget = pWidget->parentWidget();
		}
	}

	// ignore toplevels that define no context, like popups without parent
	if (!newContext.isEmpty() || qApp->focusWidget() == focusWidget()) {
		updateContextObject(newContext);
	}
}

void AssetManager::updateContextObject(const QList<IContext*>& context)
{
	emit pCoreImpl_->contextAboutToChange(context);

	activeContext_ = context;

	updateContext();

#if X_DEBUG
	if (debugLogging) 
	{
		qDebug() << "new context objects =" << context;
		foreach(IContext *c, context)
			qDebug() << (c ? c->widget() : 0) << (c ? c->widget()->metaObject()->className() : 0);
	}
#endif // !X_DEBUG
}

void AssetManager::updateContext(void)
{
	Context contexts;

	for(IContext* pContext : activeContext_) {
		contexts.add(pContext->context());
	}

	contexts.add(additionalContexts_);

	Context uniquecontexts;
	for (int32_t i = 0; i < contexts.size(); ++i) {
		const Id id = contexts.at(i);
		if (!uniquecontexts.contains(id)) {
			uniquecontexts.add(id);
		}
	}

	pActionManager_->setContext(uniquecontexts);

	emit pCoreImpl_->contextChanged(activeContext_, additionalContexts_);
}


void AssetManager::createActions(void)
{
	// File
	pNewFileAct_ = new QAction(QIcon(":/misc/img/NewFile.png"), tr("New Asset..."), this);
	pNewFileAct_->setStatusTip(tr("Create a new asset"));
	connect(pNewFileAct_, SIGNAL(triggered()), this, SLOT(newFile()));


	pSaveAllAct_ = new QAction(QIcon(":/misc/img/Saveall.png"), tr("Save all"), this);
	pSaveAllAct_->setStatusTip(tr("Save all open documents"));
	connect(pSaveAllAct_, SIGNAL(triggered()), this, SLOT(saveAll()));


	pQuitAct_ = new QAction(QIcon(":/misc/img/quit.png"), tr("Quit"), this);
	pQuitAct_->setStatusTip(tr("Quit the application"));
	connect(pQuitAct_, SIGNAL(triggered()), this, SLOT(close()));


	// View
	pViewAssetDbExpoAct_ = new QAction(tr("AssetDB Explorer"), this);


	// Window
	pWindowResetLayoutAct_ = new QAction(tr("Reset Layout"), this);
	connect(pWindowResetLayoutAct_, SIGNAL(triggered()), this, SLOT(resetLayout()));

	pReloadStyleAct_ = new QAction(tr("Reload stylesheet"), this);
	connect(pReloadStyleAct_, SIGNAL(triggered()), this, SLOT(reloadStyle()));

	// Help
	pAboutAct_ = new QAction(tr("About AssetManager"), this);
	pAboutAct_->setStatusTip(tr("Show the AssetManager's' About box"));
	connect(pAboutAct_, SIGNAL(triggered()), this, SLOT(about()));

	pAboutQtAct_ = new QAction(tr("About &Qt"), this);
	pAboutQtAct_->setStatusTip(tr("Show the Qt library's About box"));
	connect(pAboutQtAct_, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

}

void AssetManager::createMenus(void)
{
	ActionContainer* pMenuBar = ActionManager::createMenuBar(Constants::MENU_BAR);

	pMenuBar->appendGroup(Constants::G_FILE);
	pMenuBar->appendGroup(Constants::G_EDIT);
	pMenuBar->appendGroup(Constants::G_VIEW);
	pMenuBar->appendGroup(Constants::G_DEBUG);
	pMenuBar->appendGroup(Constants::G_TOOLS);
	pMenuBar->appendGroup(Constants::G_WINDOW);
	pMenuBar->appendGroup(Constants::G_HELP);

	Context globalContext(Constants::C_GLOBAL);

	ICommand* pCmd = nullptr;

	// File Menu
	{
		ActionContainer *filemenu = ActionManager::createMenu(Constants::M_FILE);
		pMenuBar->addMenu(filemenu, Constants::G_FILE);
		filemenu->menu()->setTitle(tr("File"));

		// Groups
		filemenu->appendGroup(Constants::G_FILE_NEW);
		filemenu->appendGroup(Constants::G_FILE_OPEN);
		filemenu->appendGroup(Constants::G_FILE_SAVE);
		filemenu->appendGroup(Constants::G_FILE_RECENT);
		filemenu->appendGroup(Constants::G_FILE_CLOSE);

		// File menu separators
		filemenu->addSeparator(globalContext, Constants::G_FILE_SAVE);
		filemenu->addSeparator(globalContext, Constants::G_FILE_RECENT);
		filemenu->addSeparator(globalContext, Constants::G_FILE_CLOSE);

		pCmd = ActionManager::registerAction(pNewFileAct_, Constants::NEW_ASSET, globalContext);
		pCmd->setDefaultKeySequence(QKeySequence::New);
		filemenu->addAction(pCmd, Constants::G_FILE_NEW);


		pCmd = ActionManager::registerAction(pSaveAllAct_, Constants::SAVEALL, globalContext);
		pCmd->setDefaultKeySequence(QKeySequence("Ctrl+Shift+S"));
		filemenu->addAction(pCmd, Constants::G_FILE_SAVE);

		// Exit
		pCmd = ActionManager::registerAction(pQuitAct_, Constants::EXIT, globalContext);
		filemenu->addAction(pCmd, Constants::G_FILE_CLOSE);
	}

	// View
	{
		ActionContainer *viewmenu = ActionManager::createMenu(Constants::M_VIEW);
		pMenuBar->addMenu(viewmenu, Constants::G_VIEW);
		viewmenu->menu()->setTitle(tr("View"));

		// Groups
		viewmenu->appendGroup(Constants::G_VIEW_CODE);
		viewmenu->appendGroup(Constants::G_VIEW_DOCKED);
		viewmenu->appendGroup(Constants::G_VIEW_TOOLBARS);

		// File menu separators
		viewmenu->addSeparator(globalContext, Constants::G_VIEW_CODE);
		viewmenu->addSeparator(globalContext, Constants::G_VIEW_DOCKED);
		viewmenu->addSeparator(globalContext, Constants::G_VIEW_TOOLBARS);

		// Creat Child menu's
		ActionContainer *mToolBar = ActionManager::createMenu(Constants::M_VIEW_TOOLBAR);

		mToolBar->menu()->setTitle("ToolBars");
		mToolBar->setOnAllDisabledBehavior(ActionContainer::OnAllDisabledBehavior::Show);
		viewmenu->addMenu(mToolBar, Constants::G_VIEW_TOOLBARS);


	//	// docks
	//	pCmd = ActionManager::registerAction(pViewAssetDbExpoAct_, Constants::VIEW_ASSETDBEXPLORER, globalContext);
	//	viewmenu->addAction(pCmd, Constants::G_VIEW_DOCKED);


		// connect shit
		connect(viewmenu->menu(), SIGNAL(aboutToShow()), this, SLOT(aboutToShowViewMenu()));
	}

	// Window
	{
		ActionContainer* windowmenu = ActionManager::createMenu(Constants::M_WINDOW);
		pMenuBar->addMenu(windowmenu, Constants::G_WINDOW);
		windowmenu->menu()->setTitle(tr("Window"));

		// Groups
		windowmenu->appendGroup(Constants::G_WINDOW_LAYOUT);
		windowmenu->appendGroup(Constants::G_WINDOW_SPLIT);
		windowmenu->appendGroup(Constants::G_WINDOW_DOCUMENT);
		windowmenu->appendGroup(Constants::G_WINDOW_PANES);
		windowmenu->appendGroup(Constants::G_WINDOW_WINDOWS);


		// Window menu separators
		windowmenu->addSeparator(globalContext, Constants::G_WINDOW_LAYOUT);
		windowmenu->addSeparator(globalContext, Constants::G_WINDOW_DOCUMENT);
		windowmenu->addSeparator(globalContext, Constants::G_WINDOW_WINDOWS);

		// Layout group items
		pCmd = ActionManager::registerAction(pWindowResetLayoutAct_, Constants::RESET_LAYOUT, globalContext);
		windowmenu->addAction(pCmd, Constants::G_WINDOW_LAYOUT);

		pCmd = ActionManager::registerAction(pReloadStyleAct_, Constants::RELOAD_STYLE, globalContext);
		pCmd->setDefaultKeySequence(QKeySequence::Refresh);
		windowmenu->addAction(pCmd, Constants::G_WINDOW_LAYOUT);

		// Windows group items
		ActionContainer* mWindows = ActionManager::createMenu(Constants::M_WINDOW_WINDOWS);

		mWindows->menu()->setTitle(tr("Windows"));
		mWindows->setOnAllDisabledBehavior(ActionContainer::OnAllDisabledBehavior::Show);

		windowmenu->addMenu(mWindows, Constants::G_WINDOW_WINDOWS);

		// connect shit
		connect(windowmenu->menu(), SIGNAL(aboutToShow()), this, SLOT(aboutToShowWindowMenu()));

	}

	// Help
	{
		ActionContainer *helpmenu = ActionManager::createMenu(Constants::M_HELP);
		pMenuBar->addMenu(helpmenu, Constants::G_HELP);
		helpmenu->menu()->setTitle(tr("Help"));

		// Groups
		helpmenu->appendGroup(Constants::G_HELP_HELP);
		helpmenu->appendGroup(Constants::G_HELP_ABOUT);

		// Help menu separators
		helpmenu->addSeparator(globalContext, Constants::G_HELP_HELP);
		helpmenu->addSeparator(globalContext, Constants::G_HELP_ABOUT);

		// About group items
		pCmd = ActionManager::registerAction(pAboutAct_, Constants::SHOW_ABOUT, globalContext);
		helpmenu->addAction(pCmd, Constants::G_HELP_ABOUT);


	}



	pDockArea_->setMenuBar(pMenuBar->menuBar());
}


void AssetManager::createStatusBar(void)
{

}

template<class T>
T* AssetManager::AddDockItem(const char* pName, Qt::DockWidgetAreas areas, Qt::DockWidgetArea start)
{
	QDockWidget* pDock = new QDockWidget(tr(pName), this);
	pDock->setAllowedAreas(areas);

	T* pItem = new T(pDock);

	pDock->setWidget(pItem);

	pDockArea_->addDockWidget(start, pDock);

	return pItem;
}

template<class T>
void AssetManager::AddDockItem(const char* pName, T* pWidget, Qt::DockWidgetAreas areas, Qt::DockWidgetArea start)
{
	QDockWidget* pDock = new QDockWidget(tr(pName), this);
	pDock->setAllowedAreas(areas);

	pDock->setWidget(pWidget);

	pDockArea_->addDockWidget(start, pDock);
}


void AssetManager::createDockWindows(void)
{
	pAssetViewWidget_ = new AssetExplorer::AssetDbViewWidget(*pDb_);


	Qt::DockWidgetAreas all = Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea;

	AddDockItem<AssetExplorer::AssetDbViewWidget>("Asset Explorer", pAssetViewWidget_, all, Qt::LeftDockWidgetArea);
	AddDockItem<OutputPaneManager>("Output", OutputPaneManager::instance(), all, Qt::BottomDockWidgetArea);

}

// =======================  View Menu Actions =======================

void AssetManager::aboutToShowViewMenu(void)
{
	ActionContainer* pActionContainer = ActionManager::actionContainer(Constants::M_VIEW);
	ActionContainer* pActionContainerToolBar = ActionManager::actionContainer(Constants::M_VIEW_TOOLBAR);

	pActionContainer->menu()->clear();

	{
		ActionContainer* pActionContainer = ActionManager::actionContainer(Constants::M_VIEW);

		QList<QDockWidget*> dockwidgets = pDockArea_->findChildren<QDockWidget*>();

		foreach(QDockWidget * dockwidget, dockwidgets)
		{
			pActionContainer->menu()->addAction(dockwidget->toggleViewAction());
		}
	}
	{
		QList<QToolBar*> toolbars = pDockArea_->findChildren<QToolBar *>();
		foreach(QToolBar* pBar, toolbars)
		{
			pActionContainerToolBar->menu()->addAction(pBar->toggleViewAction());
		}

		pActionContainerToolBar->menu()->setEnabled(!toolbars.empty());

		qDebug() << "Num Toolbars: " << pActionContainer->menu()->actions().size();
	}


	pActionContainer->addMenu(pActionContainerToolBar, Constants::G_VIEW_TOOLBARS);
}

// ======================= File Menu Actions =======================

void AssetManager::newFile(void)
{
	AddAssetDialog dialog(ICore::mainWindow(), *pDb_);

	dialog.exec();
}

void AssetManager::saveAll(void)
{
	AssetEntryManager::saveAllModifiedAssetEntrysSilently();
}

// =======================  Window Menu Actions =======================

void AssetManager::resetLayout(void)
{


}


void AssetManager::reloadStyle(void)
{
	reloadStyle("style\\style.qss");
}

void AssetManager::aboutToShowWindowMenu(void)
{
	// show list of open editors.
	ActionContainer* pActionContainer = ActionManager::actionContainer(Constants::M_WINDOW_WINDOWS);
	pActionContainer->menu()->clear();

	bool hasOpenFiles = false;

	foreach(IEditor* pEditor, EditorManager::openEditorsList()) 
	{
		hasOpenFiles = true;

		QAction* pAction = pActionContainer->menu()->addAction(pEditor->assetEntry()->displayName());
		pAction->setData(qVariantFromValue(pEditor));

		if (pEditor == EditorManager::currentEditor()) {
			pAction->setCheckable(true);
			pAction->setChecked(true);
		}

		connect(pAction, SIGNAL(triggered()), this, SLOT(windowListSetActiveEditor()));
	}

	pActionContainer->menu()->setEnabled(hasOpenFiles);

	// don't think i want a clear menu here since it's kinda duplicate of close all documents.
	if (hasOpenFiles)
	{
		//    aci->menu()->addSeparator();
		//    QAction *action = aci->menu()->addAction(Constants::CLEAR_MENU);
		//    connect(action, SIGNAL(triggered()), EditorManager::instance(), SLOT(closeAllEditors()));
	}
}



void AssetManager::windowListSetActiveEditor(void)
{
	QAction* pAction = qobject_cast<QAction *>(sender());
	if (pAction)
	{
		IEditor* editor = pAction->data().value<IEditor*>();

		EditorManager::activateEditor(editor);
	}
}



// =======================  Help Menu Actions =======================

void AssetManager::about(void)
{
	if (!pVersionDialog_) {
		pVersionDialog_ = new VersionDialog(this);

		connect(pVersionDialog_, SIGNAL(finished(int)), this, SLOT(destroyAboutDialog()));
	}

	pVersionDialog_->show();
}

void AssetManager::destroyAboutDialog(void)
{
	if (pVersionDialog_) {
		pVersionDialog_->deleteLater();
		pVersionDialog_ = 0;
	}
}

// =======================  Misc Slots =======================

void AssetManager::fileChanged(const QString& path)
{
	reloadStyle(path);
}

// ----------------------------------------------------


void AssetManager::closeEvent(QCloseEvent *event)
{
	// tell the goats
	if (!pCoreImpl_->callCoreCloseListners(event)) {
		return;
	}

	emit pCoreImpl_->coreAboutToClose();

	event->accept();
}

// ----------------------------------------------------


void AssetManager::reloadStyle(const QString& path)
{
	if (path.contains(".qss"))
	{
		QFile f(path);

		if (f.open(QFile::ReadOnly | QFile::Text))
		{
			QTextStream ts(&f);

			QApplication* pApp = (static_cast<QApplication *>(QCoreApplication::instance()));
			pApp->setStyleSheet(ts.readAll());

			X_LOG1("AssetManager", "style reloaded..");
		}
		else
		{
			X_ERROR("AssetManager", "Failed to load style sheet: \"%ls\"", path.data());
		}
	}
	else
	{
		X_ERROR("AssetManager", "Failed to load style sheet, invalid extension: \"%ls\"", path.data());
	}
}


X_NAMESPACE_END
