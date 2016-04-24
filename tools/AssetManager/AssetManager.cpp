#include "assetmanager.h"
#include "ui_assetmanager.h"

AssetManager::AssetManager(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AssetManager)
{
    ui->setupUi(this);
}

AssetManager::~AssetManager()
{
    delete ui;
}
