#include "modproject.h"
#include "modprojectnodes.h"


X_NAMESPACE_BEGIN(assman)

// ----------------------------------

ModProject::ModProject(AssetDB& db, const QString &name, int32_t id) :
    Project(id),
    db_(db),
    name_(name),
    modId_(id),
    rootNode_(nullptr)
{
	core::zero_object(assetTypeFolders_);

    rootNode_ = new ModProjectNode(this);

    initAssetTypeInfo();
}

ModProject::~ModProject()
{

}

void ModProject::loadAssetTypeNodes(void)
{
	assetDb::AssetDB::AssetTypeCountsArr counts;
    db_.GetAssetTypeCounts(modId_, counts);

    QList<AssetExplorer::FolderNode*> folders;

    for(size_t i=0; i<AssetType::ENUM_COUNT; i++)
    {
        if(counts[i] > 0)
        {
            const auto& info = assetDisplayInfo_[i];

            ModVirtualFolderNode* pFolder = new ModVirtualFolderNode(info.pNickName, info.priority, info.pNickName,
                static_cast<AssetType::Enum>(i), counts[i]);

            pFolder->setIcon(info.folderIcon);
            pFolder->setIconExpanded(info.folderIconExpanded);

			assetTypeFolders_[i] = pFolder;

            folders << pFolder;
        }
    }

    rootNode_->addFolderNodes(folders);
}


bool ModProject::addFile(const core::string& name, assetDb::AssetType::Enum type)
{
	// so this new item should already be in the db. 
	// we just want to add to UI.

	// if we not have this asset type before create the virtual node.
	if (assetTypeFolders_[type] == nullptr)
	{
		const auto& info = assetDisplayInfo_[type];

		ModVirtualFolderNode* pFolder = new ModVirtualFolderNode(info.pNickName, info.priority, info.pNickName,
			type, 1);


		assetTypeFolders_[type] = pFolder;

		QList<AssetExplorer::FolderNode*> folders;
		folders << pFolder;

		rootNode_->addFolderNodes(folders);
	}

	X_ASSERT_NOT_NULL(assetTypeFolders_[type]);


	return assetTypeFolders_[type]->addFile(name, type);
}

bool ModProject::removeFile(const core::string& name, assetDb::AssetType::Enum type)
{
	X_ASSERT_NOT_NULL(assetTypeFolders_[type]);

	return assetTypeFolders_[type]->removeFile(name, type);
}

QString ModProject::displayName(void) const
{
    return name_;
}

int32_t ModProject::modId(void) const
{
    return modId_;
}

AssetExplorer::ProjectNode* ModProject::rootProjectNode(void) const
{
    return rootNode_;
}


bool ModProject::getAssetList(AssetType::Enum type, core::Array<AssetInfo>& assetsOut) const
{
    return db_.GetAssetList(modId_, type, assetsOut);
}

QIcon ModProject::getIconForAssetType(AssetType::Enum type)
{
	return assetDisplayInfo_[type].icon;
}

void ModProject::initAssetTypeInfo(void)
{
    QIcon defaultFolderIcon(":/assetDb/img/Folder.Closed.png");
    QIcon defaultFolderIconExpanded(":/assetDb/img/Folder.Open.png");
	QIcon defaultIcon(":/assetDb/img/File_default.png");

    for(size_t i=0; i<AssetDB::AssetType::ENUM_COUNT; i++) {
        assetDisplayInfo_[i].pNickName = "<AssetNickName missing>";
        assetDisplayInfo_[i].priority = 0;
        assetDisplayInfo_[i].folderIcon = defaultFolderIcon;
        assetDisplayInfo_[i].folderIconExpanded = defaultFolderIconExpanded;
		assetDisplayInfo_[i].icon = defaultIcon;
    }

    typedef assetDb::AssetType assetType;

    assetDisplayInfo_[assetType::MODEL].pNickName = "Model";
    assetDisplayInfo_[assetType::MODEL].icon = QIcon(":/assetDb/img/File_mesh.png");
    assetDisplayInfo_[assetType::ANIM].pNickName = "Anim";
	assetDisplayInfo_[assetType::ANIM].icon = QIcon(":/assetDb/img/File_anim.png");
    assetDisplayInfo_[assetType::MATERIAL].pNickName = "Material";
	assetDisplayInfo_[assetType::MATERIAL].icon = QIcon(":/assetDb/img/File_material.png");
    assetDisplayInfo_[assetType::IMG].pNickName = "Images";
	assetDisplayInfo_[assetType::IMG].icon = QIcon(":/assetDb/img/File_img.png");
    assetDisplayInfo_[assetType::WEAPON].pNickName = "Weapon";
    assetDisplayInfo_[assetType::TURRET].pNickName = "Turret";
    assetDisplayInfo_[assetType::LIGHT].pNickName = "Light";
	assetDisplayInfo_[assetType::LIGHT].icon = QIcon(":/assetDb/img/File_light.png");
    assetDisplayInfo_[assetType::FX].pNickName = "Fx";
    assetDisplayInfo_[assetType::RUMBLE].pNickName = "Rumble";
    assetDisplayInfo_[assetType::SHELLSHOCK].pNickName = "ShellShock";
    assetDisplayInfo_[assetType::CHARACTER].pNickName = "Character";
	assetDisplayInfo_[assetType::CHARACTER].icon = QIcon(":/assetDb/img/File_character.png");
    assetDisplayInfo_[assetType::VEHICLE].pNickName = "Vehicle";
	assetDisplayInfo_[assetType::VEHICLE].icon = QIcon(":/assetDb/img/File_vehicle.png");
    assetDisplayInfo_[assetType::CAMERA].pNickName = "Cameras";
	assetDisplayInfo_[assetType::CAMERA].icon = QIcon(":/assetDb/img/File_cam.png");

    // can make certain asset types sort above others.
    assetDisplayInfo_[assetType::MODEL].priority = 3;
    assetDisplayInfo_[assetType::ANIM].priority = 2;
    assetDisplayInfo_[assetType::IMG].priority = 1;

}


X_NAMESPACE_END
