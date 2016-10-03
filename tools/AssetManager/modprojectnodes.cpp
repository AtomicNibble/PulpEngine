#include "modprojectnodes.h"
#include "modproject.h"


X_NAMESPACE_BEGIN(assman)


ModProjectNode::ModProjectNode(ModProject* pProject) :
    ProjectNode(""),
    pProject_(pProject)
{
    setDisplayName(pProject->displayName());

    setIcon(QIcon(":/assetDb/img/Mod_Project_Node_32.png"));
}

QList<AssetExplorer::ProjectAction> ModProjectNode::supportedActions(Node* pNode) const
{
	X_UNUSED(pNode);

	return QList<AssetExplorer::ProjectAction>()
		<< AssetExplorer::ProjectAction::AddNewFile
		<< AssetExplorer::ProjectAction::EraseFile
		<< AssetExplorer::ProjectAction::Rename;
}

bool ModProjectNode::canAddSubProject(const QString& projectName) const
{
    Q_UNUSED(projectName);
    return false;
}

bool ModProjectNode::addSubProjects(const QStringList& projectNames)
{
    Q_UNUSED(projectNames);
    return false;
}

bool ModProjectNode::removeSubProjects(const QStringList& projectNames)
{
    Q_UNUSED(projectNames);
    return false;
}



ModProject* ModProjectNode::getModProject(void)
{
    return pProject_;
}


// -------------------------------------------------------------------


ModVirtualFolderNode::ModVirtualFolderNode(const QString& name, int32_t priority, const QString& displayName,
                              AssetType::Enum assType, int32_t numAssets) :
    VirtualFolderNode(name, priority),
    displayName_(displayName),
    assetType_(assType),
    numAssets_(numAssets)
{

}

QList<AssetExplorer::ProjectAction> ModVirtualFolderNode::supportedActions(Node* pNode) const
{
	X_UNUSED(pNode);

	return QList<AssetExplorer::ProjectAction>()
		<< AssetExplorer::ProjectAction::AddNewFile
		<< AssetExplorer::ProjectAction::EraseFile
		<< AssetExplorer::ProjectAction::Rename;
}


QString ModVirtualFolderNode::displayName(void) const
{
    return tooltip();
}

QString ModVirtualFolderNode::tooltip(void) const
{
    return QString("%1 (%2)").arg(AssetType::ToString(assetType_), QString::number(numAssets_));
}


bool ModVirtualFolderNode::hasUnLoadedChildren(void) const
{
    return numAssets_ > (fileNodes_.size() + subFolderNodes_.size());
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
	//  QList<AssetExplorer::FolderNode*> folders;

      QList<ModProject::AssetInfo> assetsOut;
	  {
		  core::Array<ModProject::AssetInfo> assetsOutTmp(g_arena);
		  pProject->getAssetList(assetType_, assetsOutTmp);


	  }
	  
	  // we need to break this down into files and folders.
	  // we do it based on slashes :D
	  QChar slash(assetDb::ASSET_NAME_SLASH);


	  QIcon folderIcon(":/assetDb/img/Folder.Closed.png");
	  QIcon folderIconExpanded(":/assetDb/img/Folder.Open.png");

      for (const ModProject::AssetInfo& asset : assetsOut)
      {
          // for now just add file nodes.
		  const auto& name =  asset.name;
		  const QString qName = QString::fromUtf8(name.c_str());

		  if (name.find(assetDb::ASSET_NAME_SLASH))
		  {
#if 1
			  // make it a folder.
			 auto parts = qName.split(slash);
			 QStringListIterator it(parts);

			 AssetExplorer::FolderNode* pCurrentNode = this;

			 // we want to just populate the whole tree.
			 // when we reach a folder we need to build all the nodes.
			 // whats a good way to add the folders tho?
			 // we only expose the methods for adding nodes, as it performs parenting logic.

			 while (it.hasNext())
			 {
				 const QString& key = it.next();

				 if (it.hasNext()) // directory
				 {
					 auto pFolder = new ModFolderNode(key);
					 pFolder->setIcon(folderIcon);
					 pFolder->setIconExpanded(folderIconExpanded);


					 QList<AssetExplorer::FolderNode*> foldersChild;
					 foldersChild.append(pFolder);

					 // i want to add all these folder nodes at once.
					 // but then you can't add the children.
					 // untill they are added to parent.
					 // as they fail to get a project node.


					// pCurrentNode->addFolderNodes(foldersChild);
					// pCurrentNode = pFolder;
				 }
				 else
				 {
					 // file.
					 auto pFile = new AssetExplorer::FileNode(key, AssetExplorer::FileType::SourceType);
					 pFile->setIcon(pProject->getIconForAssetType(assetType_));

					 QList<AssetExplorer::FileNode*> filesChild;
					 filesChild.append(pFile);

					 pCurrentNode->addFileNodes(filesChild);
				 }
			 }

#endif
		  }
		  else
		  {
			  auto pFile = new AssetExplorer::FileNode(qName, AssetExplorer::FileType::SourceType);
			  pFile->setIcon(pProject->getIconForAssetType(assetType_));

			  files.append(pFile);
		  }
      }

	 this->addFileNodes(files);
	//  this->addFolderNodes(folders);
    }

    return true;
}


// -------------------------------------------------------------------


ModFolderNode::ModFolderNode(const QString &name) :
	FolderNode(name)
{

}


// -------------------------------------------------------------------


ModFileNode::ModFileNode(const QString& displayName, const QString& name, AssetType::Enum type) :
	FileNode(displayName, AssetExplorer::FileType::SourceType),
	name_(name),
	type_(type)
{

}


X_NAMESPACE_END
