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
	if (!db_->OpenDB()) {
		QMessageBox::critical(this, tr("Error"), "Failed to open AssetDB");
	}

	assetDbexplorer_ = new AssetExplorer::AssetExplorer(*db_);
	if (!assetDbexplorer_->init()) {
		QMessageBox::critical(this, tr("Error"), "Failed to init AssetExpolrer");
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


X_NAMESPACE_END
