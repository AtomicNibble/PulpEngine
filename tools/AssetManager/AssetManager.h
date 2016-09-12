#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <QMainWindow>

class QGridLayout;


X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);


namespace AssetExplorer {
    class AssetDbViewWidget;
    class AssetExplorer;
}

class AssetManager : public QMainWindow
{
    Q_OBJECT

public:
	typedef X_NAMESPACE(assetDb)::AssetDB AssetDB;

public:
    explicit AssetManager(QWidget *parent = 0);
    ~AssetManager();

private:
	bool addMod(int32_t modId, const core::string& name, core::Path<char>& outDir);


private:
    QGridLayout* layout_;

	AssetDB* db_;
    AssetExplorer::AssetDbViewWidget* assetViewWidget_;
    AssetExplorer::AssetExplorer* assetDbexplorer_;

};

#endif // ASSETMANAGER_H
