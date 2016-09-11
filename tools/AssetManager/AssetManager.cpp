#include "assetmanager.h"
#include "assetdbwidget.h"

#include "assetdbexplorer.h"
#include "assetdbnodes.h"
#include "assetdb.h"

#include "session.h"
#include "project.h"
#include "modproject.h"

#include <QtWidgets>
#include <QColor.h>
#include <QGridLayout>


AssetManager::AssetManager(QWidget *parent) :
    QMainWindow(parent),
    layout_(nullptr),
    db_(nullptr),
    assetViewWidget_(nullptr),
    assetDbexplorer_(nullptr)
{
    db_ = new AssetDb();
    db_->OpenDB("C:\\Users\\WinCat\\Documents\\code\\WinCat\\engine\\potatoengine\\game_folder\\asset_db\\potato_asset.db");

    QString errorMessage;
    assetDbexplorer_ = new AssetExplorer::AssetExplorer();
    assetDbexplorer_->init(&errorMessage);

    if (!errorMessage.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), errorMessage);
    }

    db_->IterateMods([&](AssetDb::ModId id, const QString& name, QDir& outDir)-> bool {
            Q_UNUSED(outDir);
            ModProject* pMod = new ModProject(*db_, name, id);
            pMod->loadAssetTypeNodes();
            AssetExplorer::SessionManager::addProject(pMod);
            return true;
        }
    );

    layout_ = new QGridLayout();
    layout_->addWidget(new AssetExplorer::AssetDbViewWidget(*db_));


    QWidget *window = new QWidget();
    window->setLayout(layout_);

    setCentralWidget(window);
	setMinimumSize(480, 480);


}

AssetManager::~AssetManager()
{

}
