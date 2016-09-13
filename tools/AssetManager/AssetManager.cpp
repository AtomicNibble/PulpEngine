#include "assetmanager.h"
#include "assetdbwidget.h"

#include "assetdbexplorer.h"
#include <../AssetDB/AssetDB.h>


X_NAMESPACE_BEGIN(assman)

AssetManager::AssetManager(QWidget* pParent) :
	QMainWindow(pParent),
	pLayout_(nullptr),
	pDb_(nullptr),
	assetViewWidget_(nullptr),
	assetDbexplorer_(nullptr)
{
	pDb_ = new assetDb::AssetDB();
	if (!pDb_->OpenDB()) {
		QMessageBox::critical(this, tr("Error"), "Failed to open AssetDB");
	}

	assetDbexplorer_ = new AssetExplorer::AssetExplorer(*pDb_);
	if (!assetDbexplorer_->init()) {
		QMessageBox::critical(this, tr("Error"), "Failed to init AssetExpolrer");
	}

	assetDbexplorer_->loadMods();


	pLayout_ = new QGridLayout();
	pLayout_->addWidget(new AssetExplorer::AssetDbViewWidget(*pDb_));


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

	if (assetDbexplorer_) {
		delete assetDbexplorer_;
	}
}


X_NAMESPACE_END
