#include "assetmanager.h"
#include "assetdbwidget.h"
#include "assetdbexplorer.h"
#include "ActionManager.h"
#include "EditorManager.h"
#include "ActionContainer.h"
#include "VersionDialog.h"

#include "Command.h"
#include <../AssetDB/AssetDB.h>


X_NAMESPACE_BEGIN(assman)

AssetManager::AssetManager(QWidget* pParent) :
	QMainWindow(pParent),
	pVersionDialog_(nullptr),
	pLayout_(nullptr),
	pCoreImpl_(nullptr),
	pActionManager_(nullptr),
	pEditorManager_(nullptr),
	pDb_(nullptr),
	pAssetDbexplorer_(nullptr),
	additionalContexts_(Constants::C_GLOBAL) // always have global contex
{
	pCoreImpl_ = new ICore(this);
	pActionManager_ = new ActionManager(this);


	{
		pDb_ = new assetDb::AssetDB();
		if (!pDb_->OpenDB()) {
			QMessageBox::critical(this, tr("Error"), "Failed to open AssetDB");
		}
	}

	// ----------------------------------

	createActions();
	createMenus();
	createStatusBar();

	// ----------------------------------

	// needs to be done after menu's created
	pEditorManager_ = new EditorManager(this);
	pEditorManager_->init();

	{
		pAssetDbexplorer_ = new AssetExplorer::AssetExplorer(*pDb_);
		if (!pAssetDbexplorer_->init()) {
			QMessageBox::critical(this, tr("Error"), "Failed to init AssetExpolrer");
		}
		pAssetDbexplorer_->loadMods();
	}

	{
	//	pLayout_ = new QGridLayout();
	//	pLayout_->addWidget(pAssetViewWidget_);
	//
	//	QWidget* pWindow = new QWidget();
	//	pWindow->setLayout(pLayout_);

		setCentralWidget(pEditorManager_);
		setMinimumSize(600, 800);
	}

	connect(QApplication::instance(), SIGNAL(focusChanged(QWidget*, QWidget*)),
		this, SLOT(updateFocusWidget(QWidget*, QWidget*)));


	createDockWindows();


	EditorManager::openEditor("test");

}

AssetManager::~AssetManager()
{
	if (pDb_) {
		pDb_->CloseDB();
		delete pDb_;
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
		qDebug() << "new context objects is menu";
		return;
	}

	if (debugLogging && now && now->metaObject()) {
		qDebug() << "Name: " << now->metaObject()->className();
	}


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

	if (debugLogging) 
	{
		qDebug() << "new context objects =" << context;
		foreach(IContext *c, context)
			qDebug() << (c ? c->widget() : 0) << (c ? c->widget()->metaObject()->className() : 0);
	}
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
	pSaveAllAct_ = new QAction(QIcon(":/img/Saveall.png"), tr("Save all"), this);
	pSaveAllAct_->setStatusTip(tr("Save all open documents"));
	connect(pSaveAllAct_, SIGNAL(triggered()), this, SLOT(saveAll()));


	pQuitAct_ = new QAction(QIcon(":/misc/img/quit.png"), tr("Quit"), this);
	pQuitAct_->setStatusTip(tr("Quit the application"));
	connect(pQuitAct_, SIGNAL(triggered()), this, SLOT(close()));


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

		pCmd = ActionManager::registerAction(pSaveAllAct_, Constants::SAVEALL, globalContext);
		pCmd->setDefaultKeySequence(QKeySequence("Ctrl+Shift+S"));
		filemenu->addAction(pCmd, Constants::G_FILE_SAVE);

		// Exit
		pCmd = ActionManager::registerAction(pQuitAct_, Constants::EXIT, globalContext);
		filemenu->addAction(pCmd, Constants::G_FILE_CLOSE);
	}

	// Window
	{
		ActionContainer *windowmenu = ActionManager::createMenu(Constants::M_WINDOW);
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



	setMenuBar(pMenuBar->menuBar());
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

	addDockWidget(start, pDock);

	return pItem;
}

template<class T>
void AssetManager::AddDockItem(const char* pName, T* pWidget, Qt::DockWidgetAreas areas, Qt::DockWidgetArea start)
{
	QDockWidget* pDock = new QDockWidget(tr(pName), this);
	pDock->setAllowedAreas(areas);

	pDock->setWidget(pWidget);

	addDockWidget(start, pDock);
}


void AssetManager::createDockWindows(void)
{
	pAssetViewWidget_ = new AssetExplorer::AssetDbViewWidget(*pDb_);


	Qt::DockWidgetAreas all = Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea;

	AddDockItem<AssetExplorer::AssetDbViewWidget>("Asset Explorer", pAssetViewWidget_, all, Qt::LeftDockWidgetArea);


}


void AssetManager::raiseWindow(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

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


// ----------------------------------------------------


void AssetManager::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	if (e->type() == QEvent::ActivationChange)
	{
		bool active = isActiveWindow();

		if (active) {
		//	emit windowActivated();
		}
	}
}


void AssetManager::closeEvent(QCloseEvent *event)
{
	// tell the goats
	if (!pCoreImpl_->callCoreCloseListners(event)) {
		return;
	}

	emit pCoreImpl_->coreAboutToClose();

	event->accept();
}


X_NAMESPACE_END
