#include "modprojectnodes.h"
#include "modproject.h"

#include "ConverterHost.h"

X_NAMESPACE_BEGIN(assman)


namespace
{
	class SortByName
	{
	public:
		bool operator()(AssetExplorer::Node *a, AssetExplorer::Node *b)
		{
			return operator()(a->name(), b->name());
		}
		bool operator()(AssetExplorer::Node *a, const QString &b)
		{
			return operator()(a->name(), b);
		}
		bool operator()(const QString &a, AssetExplorer::Node *b)
		{
			return operator()(a, b->name());
		}
		bool operator()(const QString &a, const QString &b)
		{
			return a < b;
		}
	};

	struct InternalNode
	{
		InternalNode()
		{
			priority = 0;
		}

		~InternalNode()
		{
			qDeleteAll(subnodes);
			qDeleteAll(virtualfolders);
		}

		void create(const core::Array<ModProject::AssetInfo>& assets)
		{
			QChar separator(assetDb::ASSET_NAME_SLASH);

			for (const ModProject::AssetInfo& asset : assets)
			{
				// for now just add file nodes.
				const auto& name = asset.name;
				const QString qName = QString::fromUtf8(name.c_str());

				InternalNode* pCurrentNode = this;

				if (name.find(assetDb::ASSET_NAME_SLASH))
				{
					auto parts = qName.split(separator);
					QStringListIterator it(parts);

					QString path;

					while (it.hasNext())
					{
						const QString& key = it.next();

						if (it.hasNext()) // directory
						{
							path += key;

							if (!pCurrentNode->subnodes.contains(path))
							{
								InternalNode* pFolder = new InternalNode;
								pFolder->fullPath = path;
								pFolder->displayName = key;
								pCurrentNode->subnodes.insert(path, pFolder);
								pCurrentNode = pFolder;
							}
							else
							{
								pCurrentNode = pCurrentNode->subnodes.value(path);
							}

							path += separator;
						}
						else
						{
							pCurrentNode->files.append(qName);
						}
					}
				}
				else
				{
					pCurrentNode->files.append(qName);
				}
			}
		}

		// Removes folder nodes with only a single sub folder in it
		void compress(void)
		{
			QMap<QString, InternalNode*> newSubnodes;
			QMapIterator<QString, InternalNode*> i(subnodes);

			while (i.hasNext())
			{
				i.next();
				i.value()->compress();

				// single sub folder?
				if (i.value()->files.isEmpty() && i.value()->subnodes.size() == 1)
				{
					QLatin1Char separator(assetDb::ASSET_NAME_SLASH);

					QString key = i.value()->subnodes.begin().key();
					InternalNode* pKeep = i.value()->subnodes.value(key);
					pKeep->displayName = i.value()->displayName + separator + pKeep->displayName;
					newSubnodes.insert(key, pKeep);
					i.value()->subnodes.clear();
					delete i.value();
				}
				else
				{
					newSubnodes.insert(i.key(), i.value());
				}
			}

			subnodes = newSubnodes;
		}


		X_INLINE AssetExplorer::FolderNode *createFolderNode(InternalNode *node)
		{
			AssetExplorer::FolderNode* newNode = nullptr;

			static QIcon folderIcon(":/assetDb/img/Folder.Closed.png");
			static QIcon folderIconExpanded(":/assetDb/img/Folder.Open.png");

			newNode = new ModFolderNode(node->fullPath);
			newNode->setIcon(folderIcon);
			newNode->setIconExpanded(folderIconExpanded);
			newNode->setDisplayName(node->displayName);

			return newNode;
		}


		void updateSubFolders(ModProject* pProject, AssetExplorer::FolderNode* pFolder, assetDb::AssetType::Enum type)
		{
			updateFiles(pProject, pFolder, type);

			// updateFolders
			QMultiMap<QString, AssetExplorer::FolderNode*> existingFolderNodes;
			{
				const auto subfolderNodes = pFolder->subFolderNodes();

				foreach(AssetExplorer::FolderNode* pNode, subfolderNodes) {
					if (pNode->nodeType() != AssetExplorer::NodeType::ProjectNodeType) {
						existingFolderNodes.insert(pNode->name(), pNode);
					}
				}
			}

			QList<AssetExplorer::FolderNode*> foldersToRemove;
			QList<AssetExplorer::FolderNode*> foldersToAdd;
			typedef QPair<InternalNode*, AssetExplorer::FolderNode*> NodePair;
			QList<NodePair> nodesToUpdate;

			// Check virtual
			{
				auto it = virtualfolders.constBegin();
				auto end = virtualfolders.constEnd();
				for (; it != end; ++it)
				{
					bool found = false;
					const QString& path = (*it)->fullPath;
					auto oldit = existingFolderNodes.constFind(path);

					while (oldit != existingFolderNodes.constEnd() && oldit.key() == path)
					{
						if (oldit.value()->nodeType() == AssetExplorer::NodeType::VirtualFolderNodeType) {
							AssetExplorer::VirtualFolderNode* vfn = qobject_cast<AssetExplorer::VirtualFolderNode *>(oldit.value());
							if (vfn->priority() == (*it)->priority) {
								found = true;
								break;
							}
						}
						++oldit;
					}

					if (found)
					{
						nodesToUpdate.append(NodePair(*it, *oldit));
					}
					else
					{
						AssetExplorer::FolderNode* pNewNode = createFolderNode(*it);
						foldersToAdd.append(pNewNode);
						nodesToUpdate.append(NodePair(*it, pNewNode));
					}
				}
			}

			// Check subnodes
			{
				auto it = subnodes.constBegin();
				auto end = subnodes.constEnd();

				for (; it != end; ++it)
				{
					bool found = false;
					QString path = it.value()->fullPath;

					auto oldit = existingFolderNodes.constFind(path);

					while (oldit != existingFolderNodes.constEnd() && oldit.key() == path)
					{
						if (oldit.value()->nodeType() == AssetExplorer::NodeType::FolderNodeType) {
							found = true;
							break;
						}
						++oldit;
					}

					if (found)
					{
						nodesToUpdate << NodePair(it.value(), *oldit);
					}
					else
					{
						AssetExplorer::FolderNode *newNode = createFolderNode(it.value());
						foldersToAdd << newNode;
						nodesToUpdate << NodePair(it.value(), newNode);
					}
				}
			}

			QSet<AssetExplorer::FolderNode*> toKeep;
			toKeep.reserve(nodesToUpdate.size());

			foreach(const NodePair& np, nodesToUpdate) {
				toKeep.insert(np.second);
			}

			auto jit = existingFolderNodes.constBegin();
			auto jend = existingFolderNodes.constEnd();
			for (; jit != jend; ++jit) {
				if (!toKeep.contains(jit.value())) {
					foldersToRemove << jit.value();
				}
			}

			if (!foldersToRemove.isEmpty()) {
				pFolder->removeFolderNodes(foldersToRemove);
			}
			if (!foldersToAdd.isEmpty()) {
				pFolder->addFolderNodes(foldersToAdd);
			}

			foreach(const NodePair &np, nodesToUpdate) {
				np.first->updateSubFolders(pProject, np.second, type);
			}
		}

		void updateFiles(ModProject* pProject, AssetExplorer::FolderNode* pFolder, assetDb::AssetType::Enum type)
		{
			QList<AssetExplorer::FileNode*> existingFileNodes = pFolder->fileNodes();
			QList<AssetExplorer::FileNode*> filesToRemove;
			QStringList filesToAdd;

			SortByName sortByName;
			qSort(files.begin(), files.end(), sortByName);
			qSort(existingFileNodes.begin(), existingFileNodes.end(), sortByName);

			compareSortedLists(existingFileNodes, files, filesToRemove, filesToAdd, sortByName);

			QList<AssetExplorer::FileNode *> nodesToAdd;
			nodesToAdd.reserve(filesToAdd.size());
			foreach(const QString& file, filesToAdd) {

				ModFileNode* pFileNode = new ModFileNode(file, file, type);
				pFileNode->setIcon(pProject->getIconForAssetType(type));

				nodesToAdd.append(pFileNode);
			}

			pFolder->removeFileNodes(filesToRemove);
			pFolder->addFileNodes(nodesToAdd);
		}

		void insertSubFolders(ModProject* pProject, AssetExplorer::FolderNode* pFolder, assetDb::AssetType::Enum type)
		{
			insertFiles(pProject, pFolder, type);


			// updateFolders
			QMultiMap<QString, AssetExplorer::FolderNode*> existingFolderNodes;
			{
				const auto subfolderNodes = pFolder->subFolderNodes();

				foreach(AssetExplorer::FolderNode* pNode, subfolderNodes) {
					if (pNode->nodeType() != AssetExplorer::NodeType::ProjectNodeType) {
						existingFolderNodes.insert(pNode->name(), pNode);
					}
				}
			}

			QList<AssetExplorer::FolderNode*> foldersToAdd;
			typedef QPair<InternalNode*, AssetExplorer::FolderNode*> NodePair;
			QList<NodePair> nodesToUpdate;

			// Check virtual
			{
				auto it = virtualfolders.constBegin();
				auto end = virtualfolders.constEnd();
				for (; it != end; ++it)
				{
					bool found = false;
					const QString& path = (*it)->fullPath;
					auto oldit = existingFolderNodes.constFind(path);

					while (oldit != existingFolderNodes.constEnd() && oldit.key() == path)
					{
						if (oldit.value()->nodeType() == AssetExplorer::NodeType::VirtualFolderNodeType) {
							AssetExplorer::VirtualFolderNode* vfn = qobject_cast<AssetExplorer::VirtualFolderNode *>(oldit.value());
							if (vfn->priority() == (*it)->priority) {
								found = true;
								break;
							}
						}
						++oldit;
					}

					if (found)
					{
						nodesToUpdate.append(NodePair(*it, *oldit));
					}
					else
					{
						AssetExplorer::FolderNode* pNewNode = createFolderNode(*it);
						foldersToAdd.append(pNewNode);
						nodesToUpdate.append(NodePair(*it, pNewNode));
					}
				}
			}

			// Check subnodes
			{
				auto it = subnodes.constBegin();
				auto end = subnodes.constEnd();

				for (; it != end; ++it)
				{
					bool found = false;
					QString path = it.value()->fullPath;

					auto oldit = existingFolderNodes.constFind(path);

					while (oldit != existingFolderNodes.constEnd() && oldit.key() == path)
					{
						if (oldit.value()->nodeType() == AssetExplorer::NodeType::FolderNodeType) {
							found = true;
							break;
						}
						++oldit;
					}

					if (found)
					{
						nodesToUpdate << NodePair(it.value(), *oldit);
					}
					else
					{
						AssetExplorer::FolderNode *newNode = createFolderNode(it.value());
						foldersToAdd << newNode;
						nodesToUpdate << NodePair(it.value(), newNode);
					}
				}
			}

			QSet<AssetExplorer::FolderNode*> toKeep;
			toKeep.reserve(nodesToUpdate.size());

			foreach(const NodePair& np, nodesToUpdate) {
				toKeep.insert(np.second);
			}

			if (!foldersToAdd.isEmpty()) {
				pFolder->addFolderNodes(foldersToAdd);
			}

			foreach(const NodePair &np, nodesToUpdate) {
				np.first->insertSubFolders(pProject, np.second, type);
			}

		}

		void removeSubFolders(ModProject* pProject, AssetExplorer::FolderNode* pFolder, assetDb::AssetType::Enum type)
		{
			updateFiles(pProject, pFolder, type);

			// updateFolders
			QMultiMap<QString, AssetExplorer::FolderNode*> existingFolderNodes;
			{
				const auto subfolderNodes = pFolder->subFolderNodes();

				foreach(AssetExplorer::FolderNode* pNode, subfolderNodes) {
					if (pNode->nodeType() != AssetExplorer::NodeType::ProjectNodeType) {
						existingFolderNodes.insert(pNode->name(), pNode);
					}
				}
			}

			QList<AssetExplorer::FolderNode*> foldersToRemove;
			typedef QPair<InternalNode*, AssetExplorer::FolderNode*> NodePair;
			QList<NodePair> nodesToUpdate;

			// Check virtual
			{
				auto it = virtualfolders.constBegin();
				auto end = virtualfolders.constEnd();
				for (; it != end; ++it)
				{
					bool found = false;
					const QString& path = (*it)->fullPath;
					auto oldit = existingFolderNodes.constFind(path);

					while (oldit != existingFolderNodes.constEnd() && oldit.key() == path)
					{
						if (oldit.value()->nodeType() == AssetExplorer::NodeType::VirtualFolderNodeType) {
							AssetExplorer::VirtualFolderNode* vfn = qobject_cast<AssetExplorer::VirtualFolderNode *>(oldit.value());
							if (vfn->priority() == (*it)->priority) {
								found = true;
								break;
							}
						}
						++oldit;
					}

					if (found)
					{
						nodesToUpdate.append(NodePair(*it, *oldit));
					}
				}
			}

			// Check subnodes
			{
				auto it = subnodes.constBegin();
				auto end = subnodes.constEnd();

				for (; it != end; ++it)
				{
					bool found = false;
					QString path = it.value()->fullPath;

					auto oldit = existingFolderNodes.constFind(path);

					while (oldit != existingFolderNodes.constEnd() && oldit.key() == path)
					{
						if (oldit.value()->nodeType() == AssetExplorer::NodeType::FolderNodeType) {
							found = true;
							break;
						}
						++oldit;
					}

					if (found)
					{
						nodesToUpdate << NodePair(it.value(), *oldit);
					}
				}
			}

			// we only want to remove a folder if it's empty
			// and it's the parent of a file we deleting.

			auto jit = existingFolderNodes.constBegin();
			auto jend = existingFolderNodes.constEnd();
			for (; jit != jend; ++jit) {

					foldersToRemove << jit.value();
				
			}

			if (!foldersToRemove.isEmpty()) {
		//		pFolder->removeFolderNodes(foldersToRemove);
			}

			foreach(const NodePair &np, nodesToUpdate) {
				np.first->removeSubFolders(pProject, np.second, type);
			}

		}

		void insertFiles(ModProject* pProject, AssetExplorer::FolderNode* pFolder, assetDb::AssetType::Enum type)
		{
			QList<AssetExplorer::FileNode*> existingFileNodes = pFolder->fileNodes();
			QList<AssetExplorer::FileNode*> filesToRemove;
			QStringList filesToAdd;

			SortByName sortByName;
			qSort(files.begin(), files.end(), sortByName);
			qSort(existingFileNodes.begin(), existingFileNodes.end(), sortByName);

			compareSortedLists(existingFileNodes, files, filesToRemove, filesToAdd, sortByName);

			QList<AssetExplorer::FileNode *> nodesToAdd;
			nodesToAdd.reserve(filesToAdd.size());
			foreach(const QString& file, filesToAdd)
			{
				ModFileNode* pFileNode = new ModFileNode(file, file, type);
				pFileNode->setIcon(pProject->getIconForAssetType(type));

				nodesToAdd.append(pFileNode);
			}

			pFolder->addFileNodes(nodesToAdd);
		}

	private:
		QList<InternalNode *> virtualfolders;
		QMap<QString, InternalNode *> subnodes;
		QStringList files;
		int32_t priority;
		QString displayName;
		QString fullPath;
	};


} // namespace


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
	X_ASSERT_UNREACHABLE();
    return false;
}

bool ModProjectNode::removeSubProjects(const QStringList& projectNames)
{
    Q_UNUSED(projectNames);
	X_ASSERT_UNREACHABLE();
    return false;
}

bool ModProjectNode::addFile(const core::string& name, assetDb::AssetType::Enum type)
{
	return pProject_->addFile(name, type);
}

bool ModProjectNode::removeFile(const core::string& name, assetDb::AssetType::Enum type)
{
	return pProject_->removeFile(name, type);
}

bool ModProjectNode::clean(ConverterHost& conHost) const
{
	int32_t modId = pProject_->modId();

	// les warn, might be miss click.
	const auto result = QMessageBox::warning(ICore::mainWindow(), "Clean Mod", 
		"Confirm Clean mod.\n\nThis will delete all coverted assets for the selected mod",
		QMessageBox::Ok | QMessageBox::Cancel,
		QMessageBox::Cancel);
	
	if (result == QMessageBox::Ok) {
		conHost.cleanMod(modId);
	}

	return true;
}

ModProject* ModProjectNode::getModProject(void)
{
    return pProject_;
}

bool ModProjectNode::build(ConverterHost& conHost, bool force) const
{
	int32_t modId = pProject_->modId();

	conHost.convertMod(modId, force);
	return true;
}

// -------------------------------------------------------------------


ModVirtualFolderNode::ModVirtualFolderNode(const QString& name, int32_t priority, const QString& displayName,
                              AssetType::Enum assType, int32_t numAssets) :
	AssetTypeVirtualFolderNode(name, priority, assType),
    displayName_(displayName),
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
    return QString("%1s (%2)").arg(Utils::capitalize(AssetType::ToString(assetType())), QString::number(numAssets_));
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


	  core::Array<ModProject::AssetInfo> assetsOutTmp(g_arena);
	  if (!pProject->getAssetList(assetType(), assetsOutTmp)) {
		  return false;
	  }

	  InternalNode contents;
	  contents.create(assetsOutTmp);
	  contents.compress(); // this actually makes adding nodes faster when we have a lot of folder with one node.
	  contents.updateSubFolders(pProject, this, assetType());

    }

    return true;
}

bool ModVirtualFolderNode::addFile(const core::string& name, assetDb::AssetType::Enum type)
{
	// inc asset count.
	++numAssets_;


	ModProjectNode* pProjectNode = qobject_cast<ModProjectNode*>(projectNode());
	if (pProjectNode)
	{
		ModProject* pProject = pProjectNode->getModProject();

		core::Array<ModProject::AssetInfo> assetsOutTmp(g_arena);
		// maybe get this from the asset db :/
		auto& info = assetsOutTmp.AddOne();
		info.id = assetDb::AssetDB::INVALID_ASSET_ID;
		info.parentId = assetDb::AssetDB::INVALID_ASSET_ID;
		info.name = name;
		info.type = type;

		InternalNode contents;
		contents.create(assetsOutTmp);
		contents.compress(); 
		contents.insertSubFolders(pProject, this, assetType());
	}

	return true;
}

bool ModVirtualFolderNode::removeFile(const core::string& name, assetDb::AssetType::Enum type)
{
	// dec asset count.
	--numAssets_;

	ModProjectNode* pProjectNode = qobject_cast<ModProjectNode*>(projectNode());
	if (pProjectNode)
	{
#if 1
	X_UNUSED(name);
	X_UNUSED(type);
#else
		ModProject* pProject = pProjectNode->getModProject();

		core::Array<ModProject::AssetInfo> assetsOutTmp(g_arena);
		// maybe get this from the asset db :/
		auto& info = assetsOutTmp.AddOne();
		info.id = assetDb::AssetDB::INVALID_ASSET_ID;
		info.parentId = assetDb::AssetDB::INVALID_ASSET_ID;
		info.name = name;
		info.type = type;

		InternalNode contents;
		contents.create(assetsOutTmp);
		contents.compress();
		contents.removeSubFolders(pProject, this, assetType());
#endif
	}

	return true;
}


bool ModVirtualFolderNode::build(ConverterHost& conHost, bool force) const
{
	ModProjectNode* pProjectNode = qobject_cast<ModProjectNode*>(projectNode());
	if (!pProjectNode) {
		return false;
	}

	ModProject* pProject = pProjectNode->getModProject();
	int32_t modId = pProject->modId();

	conHost.convertMod(modId, assetType(), force);
	return true;
}

// -------------------------------------------------------------------


ModFolderNode::ModFolderNode(const QString &name) :
	FolderNode(name)
{

}

bool ModFolderNode::build(ConverterHost& conHost, bool force) const
{
	const auto folders = subFolderNodes();
	for (const auto& f : folders)
	{
		if (!f->build(conHost, force)) {
			// we should probs have a setting that allow continue on single build failure.
			// but actually this is not building but scheduling build.
			return false;
		}
	}

	const auto files = fileNodes();
	for (const auto& f : files)
	{
		if (!f->build(conHost, force)) {
			return false;
		}
	}

	return true;
}

// -------------------------------------------------------------------


ModFileNode::ModFileNode(const QString& displayName, const QString& name, AssetType::Enum type) :
	FileNode(displayName, type),
	name_(name)
{

}

bool ModFileNode::build(ConverterHost& conHost, bool force) const
{
	const auto array = name().toLocal8Bit();
	core::string nameNarrow(array.data());

	conHost.convertAsset(nameNarrow, assetType(), force);
	return true;
}

X_NAMESPACE_END
