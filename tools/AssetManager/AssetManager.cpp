#include "assetmanager.h"
#include "assetdbwidget.h"

#include "assetdbexplorer.h"
#include <../AssetDB/AssetDB.h>


X_NAMESPACE_BEGIN(assman)

AssetManager::AssetManager(QWidget* pParent) :
	QMainWindow(pParent),
	pLayout_(nullptr),
	pDb_(nullptr),
	pAssetDbexplorer_(nullptr)
{
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


X_NAMESPACE_END
