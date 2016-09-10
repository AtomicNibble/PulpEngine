#include "modprojectnodes.h"
#include "modproject.h"

ModProjectNode::ModProjectNode(ModProject* pProject) :
    ProjectNode(""),
    pProject_(pProject)
{
    setDisplayName(pProject->displayName());

    setIcon(QIcon(":/assetDb/img/Mod_Project_Node_32.png"));
}


bool ModProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath);
    return false;
}

bool ModProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths);
    return false;
}

bool ModProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths);
    return false;
}



ModProject* ModProjectNode::getModProject(void)
{
    return pProject_;
}


// -------------------------------------------------------------------


ModVirtualFolderNode::ModVirtualFolderNode(const QString &name, int priority, const QString& displayName,
                              AssetType::Enum assType, int32_t numAssets) :
    VirtualFolderNode(name, priority),
    displayName_(displayName),
    assetType_(assType),
    numAssets_(numAssets)
{

}

QString ModVirtualFolderNode::displayName() const
{
    return displayName_;
}

QString ModVirtualFolderNode::tooltip() const
{
    return QString("%1 (%2)").arg(AssetType::ToString(assetType_), QString::number(numAssets_));
}


bool ModVirtualFolderNode::hasUnLoadedChildren(void) const
{
    return numAssets_ > fileNodes_.size();
}

bool ModVirtualFolderNode::loadChildren(void)
{
    ModProjectNode* pProjectNode = qobject_cast<ModProjectNode*>(projectNode());
    if(pProjectNode)
    {
      // the mod project knows has the db handle we need to query.
      // should we just ask it for the list of files?
      // we basically need to make a fileNodes list and foldersNode list.
      ModProject* pProject = pProjectNode->getModProject();

      QList<AssetExplorer::FileNode*> files;
      QList<ModProject::AssetInfo> assetsOut;
      pProject->getAssetList(assetType_, assetsOut);

      foreach (const ModProject::AssetInfo& asset, assetsOut)
      {
        // for now just add file nodes.

        files.append(new AssetExplorer::FileNode(asset.name, AssetExplorer::FileType::SourceType));
      }

      this->addFileNodes(files);
    }
    return true;
}
