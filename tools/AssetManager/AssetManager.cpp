#include "assetmanager.h"
#include "assetdbwidget.h"
#include "assetdbexplorer.h"
#include "ActionManager.h"



#include <../AssetDB/AssetDB.h>


X_NAMESPACE_BEGIN(assman)

AssetManager::AssetManager(QWidget* pParent) :
	QMainWindow(pParent),
	pLayout_(nullptr),
	pCoreImpl_(nullptr),
	pActionManager_(nullptr),
	pDb_(nullptr),
	pAssetDbexplorer_(nullptr)
{
	pCoreImpl_ = new ICore(this);
	pActionManager_ = new ActionManager(this);


	pDb_ = new assetDb::AssetDB();
	if (!pDb_->OpenDB()) {
		QMessageBox::critical(this, tr("Error"), "Failed to open AssetDB");
	}

	pAssetDbexplorer_ = new AssetExplorer::AssetExplorer(*pDb_);
	if (!pAssetDbexplorer_->init()) {
		QMessageBox::critical(this, tr("Error"), "Failed to init AssetExpolrer");
	}
	pAssetDbexplorer_->loadMods();


	pAssetViewWidget_ = new AssetExplorer::AssetDbViewWidget(*pDb_);


	pLayout_ = new QGridLayout();
	pLayout_->addWidget(pAssetViewWidget_);


	QWidget* pWindow = new QWidget();
	pWindow->setLayout(pLayout_);

	setCentralWidget(pWindow);
	setMinimumSize(600, 800);



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

}

void AssetManager::createStatusBar(void)
{

}

void AssetManager::createDockWindows(void)
{

}


void AssetManager::raiseWindow(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}


X_NAMESPACE_END
