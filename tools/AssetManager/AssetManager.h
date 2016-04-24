#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <QMainWindow>

namespace Ui {
class AssetManager;
}

class AssetManager : public QMainWindow
{
    Q_OBJECT

public:
    explicit AssetManager(QWidget *parent = 0);
    ~AssetManager();

private:
    Ui::AssetManager *ui;
};

#endif // ASSETMANAGER_H
