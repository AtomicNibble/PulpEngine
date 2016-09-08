#include "modproject.h"
#include "modprojectnodes.h"

#include "assetdb.h"


// ----------------------------------

ModProject::ModProject(AssetDb& db, const QString &name, int32_t id) :
    Project(id),
    db_(db),
    name_(name),
    modId_(id),
    rootNode_(nullptr)
{

    rootNode_ = new ModProjectNode(this);

    initAssetTypeInfo();
}

ModProject::~ModProject()
{

}

void ModProject::loadAssetTypeNodes(void)
{
    AssetDb::AssetTypeCountsArray counts;
    db_.getAssetTypeCounts(modId_, counts);

    QList<AssetExplorer::FolderNode*> folders;

    for(size_t i=0; i<AssetDb::AssetType::ENUM_COUNT; i++)
    {
        if(counts[i] > 0)
        {
            const auto& info = assetDisplayInfo_[i];

            ModVirtualFolderNode* pFolder = new ModVirtualFolderNode(info.pNickName, 0, info.pNickName,
                static_cast<AssetDb::AssetType::Enum>(i), counts[i]);

            pFolder->setIcon(info.icon);
            pFolder->setIconExpanded(info.iconExpanded);

            folders << pFolder;
        }
    }

    rootNode_->addFolderNodes(folders);
}

QString ModProject::displayName(void) const
{
    return name_;
}

int32_t ModProject::modId(void) const
{
    return modId_;
}

AssetExplorer::ProjectNode* ModProject::rootProjectNode() const
{
    return rootNode_;
}


void ModProject::initAssetTypeInfo(void)
{
    QIcon defaultIcon(":/assetDb/img/Folder.Closed.png");
    QIcon defaultIconExpanded(":/assetDb/img/Folder.Open.png");

    for(size_t i=0; i<AssetDb::AssetType::ENUM_COUNT; i++) {
        assetDisplayInfo_[i].pNickName = "<AssetNickName missing>";
        assetDisplayInfo_[i].icon = defaultIcon;
        assetDisplayInfo_[i].iconExpanded = defaultIconExpanded;
    }

    typedef X_NAMESPACE(assetDb)::AssetType assetType;

    assetDisplayInfo_[assetType::MODEL].pNickName = "Model";
  //  assetDisplayInfo_[assetType::MODEL].icon = QIcon(":/assetDb/img/File_mesh.png");
    assetDisplayInfo_[assetType::ANIM].pNickName = "Anim";
    assetDisplayInfo_[assetType::MATERIAL].pNickName = "Material";
    assetDisplayInfo_[assetType::IMG].pNickName = "Images";
    assetDisplayInfo_[assetType::WEAPON].pNickName = "Weapon";
    assetDisplayInfo_[assetType::TURRET].pNickName = "Turret";
    assetDisplayInfo_[assetType::LIGHT].pNickName = "Light";
    assetDisplayInfo_[assetType::FX].pNickName = "Fx";
    assetDisplayInfo_[assetType::RUMBLE].pNickName = "Rumble";
    assetDisplayInfo_[assetType::SHELLSHOCK].pNickName = "ShellShock";
    assetDisplayInfo_[assetType::CHARACTER].pNickName = "Character";
    assetDisplayInfo_[assetType::VEHICLE].pNickName = "Vehicle";
    assetDisplayInfo_[assetType::CAMERA].pNickName = "Cameras";

}
