#include "assetmanager.h"
#include "assetdbwidget.h"

#include "assetdbexplorer.h"

#include <QtWidgets>
#include <QColor.h>
#include <QGridLayout>


AssetManager::AssetManager(QWidget *parent) :
    QMainWindow(parent)
{
    assetDbexplorer_ = new AssetExplorer::AssetExplorer();

    QString errorMessage;
    assetDbexplorer_->init(&errorMessage);

    // add base project.


    layout_ = new QGridLayout();
    layout_->addWidget(new AssetExplorer::AssetDbViewWidget());

    QWidget *window = new QWidget();
    window->setLayout(layout_);

    setCentralWidget(window);
}

AssetManager::~AssetManager()
{

}
