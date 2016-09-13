#include "assetmanager.h"
#include "assetdbwidget.h"

#include "assetdbexplorer.h"
#include "assetdbnodes.h"
#include <../AssetDB/AssetDB.h>

#include "session.h"
#include "project.h"
#include "modproject.h"

#include <QtWidgets>
#include <QColor.h>
#include <QGridLayout>

X_NAMESPACE_BEGIN(assman)

AssetManager::AssetManager(QWidget *parent) :
    QMainWindow(parent),
    layout_(nullptr),
    db_(nullptr),
    assetViewWidget_(nullptr),
    assetDbexplorer_(nullptr)
{
    db_ = new assetDb::AssetDB();
    db_->OpenDB();

    QString errorMessage;
    assetDbexplorer_ = new AssetExplorer::AssetExplorer();
    assetDbexplorer_->init(&errorMessage);

    if (!errorMessage.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), errorMessage);
    }
	
	{
		core::Delegate<bool(assetDb::AssetDB::ModId id, const core::string& name, core::Path<char>& outDir)> func;
		func.Bind<AssetManager, &AssetManager::addMod>(this);
		db_->IterateMods(func);
	}
	

    layout_ = new QGridLayout();
    layout_->addWidget(new AssetExplorer::AssetDbViewWidget(*db_));


    QWidget *window = new QWidget();
    window->setLayout(layout_);

    setCentralWidget(window);
	setMinimumSize(600, 800);
}

AssetManager::~AssetManager()
{

}


bool AssetManager::addMod(AssetDB::ModId modId, const core::string& name, core::Path<char>& outDir)
{
	Q_UNUSED(outDir);

	ModProject* pMod = new ModProject(*db_, QString::fromUtf8(name.c_str()), modId);
	pMod->loadAssetTypeNodes();
	AssetExplorer::SessionManager::addProject(pMod);

	return true;
}

X_NAMESPACE_END
