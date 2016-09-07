#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <QMainWindow>

class QGridLayout;
class AssetDb;

namespace AssetExplorer {
    class AssetDbViewWidget;
    class AssetExplorer;
}

class AssetManager : public QMainWindow
{
    Q_OBJECT

public:
    explicit AssetManager(QWidget *parent = 0);
    ~AssetManager();

private:
    QGridLayout* layout_;

    AssetDb* db_;
    AssetExplorer::AssetDbViewWidget* assetViewWidget_;
    AssetExplorer::AssetExplorer* assetDbexplorer_;

};

#endif // ASSETMANAGER_H
