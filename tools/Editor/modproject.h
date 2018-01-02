#ifndef MODPROJECT_H
#define MODPROJECT_H


#include "project.h"
#include <../AssetDB/AssetDB.h>


X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(editor)

class ModProjectNode;
class ModVirtualFolderNode;

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

	typedef std::array<AssetTypeInfo, assetDb::AssetType::ENUM_COUNT> AssetTypeInfoArr;

public:
	typedef assetDb::AssetType AssetType;
	typedef assetDb::AssetDB::AssetInfo AssetInfo;
	typedef assetDb::AssetDB AssetDB;

public:
	ModProject(AssetDB& db, const QString &name, int32_t id);
	~ModProject() override;

	void loadAssetTypeNodes(void);

	bool addFile(const core::string& name, assetDb::AssetType::Enum type);
	bool removeFile(const core::string& name, assetDb::AssetType::Enum type);

	QString displayName(void) const override;
	int32_t modId(void) const;
	AssetExplorer::ProjectNode* rootProjectNode(void) const override;

	bool getAssetList(AssetType::Enum type, core::Array<AssetInfo>& assetsOut) const;

	QIcon getIconForAssetType(AssetType::Enum type);

private:
	void initAssetTypeInfo(void);

private:
	AssetDB& db_;
	QString name_;
	int32_t modId_;
	ModProjectNode* rootNode_;
	ModVirtualFolderNode* assetTypeFolders_[AssetType::ENUM_COUNT];

	AssetTypeInfoArr assetDisplayInfo_;
};

X_NAMESPACE_END


#endif // MODPROJECT_H
