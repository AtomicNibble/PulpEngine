#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <QMainWindow>

class QGridLayout;

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(assman)

namespace AssetExplorer {
    class AssetDbViewWidget;
    class AssetExplorer;
}

class AssetManager : public QMainWindow
{
    Q_OBJECT

public:
    explicit AssetManager(QWidget* pParent = nullptr);
    ~AssetManager();


private:
    QGridLayout* pLayout_;

	assetDb::AssetDB* pDb_;
	AssetExplorer::AssetDbViewWidget* pAssetViewWidget_;
    AssetExplorer::AssetExplorer* pAssetDbexplorer_;

};

X_NAMESPACE_END

#endif // ASSETMANAGER_H
