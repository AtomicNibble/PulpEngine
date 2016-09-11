#ifndef MODPROJECT_H
#define MODPROJECT_H


#include <QObject>
#include <QIcon>

#include <array>

#include "project.h"
#include "assetdb.h"

class ModProjectNode;
class AssetDb;

class ModProject : public AssetExplorer::Project
{
    Q_OBJECT

    struct AssetTypeInfo
    {
        const char* pNickName;
        int priority; // for sorting.
		QIcon icon;
        QIcon folderIcon;
        QIcon folderIconExpanded;
    };

    typedef std::array<AssetTypeInfo, X_NAMESPACE(assetDb)::AssetType::ENUM_COUNT> AssetTypeInfoArr;

public:
    typedef X_NAMESPACE(assetDb)::AssetType AssetType;
    typedef AssetDb::AssetInfo AssetInfo;

public:
    ModProject(AssetDb& db, const QString &name, int32_t id);
    ~ModProject() override;

    void loadAssetTypeNodes(void);

    QString displayName(void) const override;
    int32_t modId(void) const;
    AssetExplorer::ProjectNode* rootProjectNode(void) const override;

    bool getAssetList(AssetType::Enum type, QList<AssetInfo>& assetsOut) const;

	QIcon getIconForAssetType(AssetType::Enum type);

private:
    void initAssetTypeInfo(void);

private:
    AssetDb& db_;
    QString name_;
    int32_t modId_;
    ModProjectNode* rootNode_;

    AssetTypeInfoArr assetDisplayInfo_;
};



#endif // MODPROJECT_H
