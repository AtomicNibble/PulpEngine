#include "assetdbmodel.h"
#include "assert_qt.h"

#include "assetdbnodes.h"

#include <QSqlQueryModel>
#include <QDebug>
#include <QTreeView>
#include <QFileInfo>
#include <QStringList>

X_NAMESPACE_BEGIN(editor)

namespace AssetExplorer
{

namespace
{

	int caseFriendlyCompare(const QString &a, const QString &b)
	{
		int result = a.compare(b, Qt::CaseInsensitive);
		if (result != 0) {
			return result;
		}
		return a.compare(b, Qt::CaseSensitive);
	}

	bool sortNodes(Node *n1, Node *n2)
	{
		// Ordering is: project files, project, folder, file

		const NodeType n1Type = n1->nodeType();
		const NodeType n2Type = n2->nodeType();

		// projects
		if (n1Type == NodeType::ProjectNodeType) {
			if (n2Type == NodeType::ProjectNodeType) {
				ProjectNode *project1 = static_cast<ProjectNode*>(n1);
				ProjectNode *project2 = static_cast<ProjectNode*>(n2);

				int result = caseFriendlyCompare(project1->displayName(), project2->displayName());
				if (result != 0)
					return result < 0;
				else
					return project1 < project2; // sort by pointer value
			} else {
			   return true; // project is before folder & file
		   }
		}
		if (n2Type == NodeType::ProjectNodeType)
			return false;

		if (n1Type == NodeType::VirtualFolderNodeType) {
			if (n2Type == NodeType::VirtualFolderNodeType) {
				VirtualFolderNode *folder1 = static_cast<VirtualFolderNode *>(n1);
				VirtualFolderNode *folder2 = static_cast<VirtualFolderNode *>(n2);

				if (folder1->priority() > folder2->priority())
					return true;
				if (folder1->priority() < folder2->priority())
					return false;
				int result = caseFriendlyCompare(folder1->name(), folder2->name());
				if (result != 0)
					return result < 0;
				else
					return folder1 < folder2;
			} else {
				return true; // virtual folder is before folder
			}
		}

		if (n2Type == NodeType::VirtualFolderNodeType)
			return false;


		if (n1Type == NodeType::FolderNodeType) {
			if (n2Type == NodeType::FolderNodeType) {
				FolderNode *folder1 = static_cast<FolderNode*>(n1);
				FolderNode *folder2 = static_cast<FolderNode*>(n2);

				int result = caseFriendlyCompare(folder1->name(), folder2->name());
				if (result != 0)
					return result < 0;
				else
					return folder1 < folder2;
			} else {
				return true; // folder is before file
			}
		}
		if (n2Type == NodeType::FolderNodeType)
			return false;

		// must be file nodes
		{
			const QString fileName1 = n1->name();
			const QString fileName2 = n2->name();

			int result = caseFriendlyCompare(fileName1, fileName2);

			return result < 0; // sort by filename
		}
		return false;
	}


	bool isSorted(const QList<Node*>& nodes)
	{
		const int32_t size = nodes.size();
		for (int32_t i = 0; i < size - 1; ++i) {
			if (!sortNodes(nodes.at(i), nodes.at(i + 1))) {
				return false;
			}
		}
		return true;
	}

} // namnespace



AssetDBModel::AssetDBModel(SessionNode* pRootNode, AssetDB& db, QObject* pParent) :
    QAbstractItemModel(pParent),
    pTreeview_(nullptr),
    pStartupProject_(nullptr),
    pRootNode_(pRootNode),
    db_(db)
{
	NodesWatcher* pWatcher = new NodesWatcher(this);
	pRootNode_->registerWatcher(pWatcher);


	connect(pWatcher, SIGNAL(foldersAboutToBeAdded(FolderNode*, QList<FolderNode*>)),
		this, SLOT(foldersAboutToBeAdded(FolderNode*, QList<FolderNode*>)));
	connect(pWatcher, SIGNAL(foldersAdded()),
		this, SLOT(foldersAdded()));

	connect(pWatcher, SIGNAL(filesAboutToBeAdded(FolderNode*, QList<FileNode*>)),
		this, SLOT(filesAboutToBeAdded(FolderNode*, QList<FileNode*>)));
	connect(pWatcher, SIGNAL(filesAdded()),
		this, SLOT(filesAdded()));

}

AssetDBModel::~AssetDBModel()
{

}

void AssetDBModel::setTreeView(QTreeView* pTree)
{
    pTreeview_ = pTree;
}

void AssetDBModel::setStartupProject(ProjectNode* pProjectNode)
{
    if (pStartupProject_ != pProjectNode) {
        QModelIndex oldIndex = (pStartupProject_ ? indexForNode(pStartupProject_) : QModelIndex());
        QModelIndex newIndex = (pProjectNode ? indexForNode(pProjectNode) : QModelIndex());
        pStartupProject_ = pProjectNode;
        if (oldIndex.isValid()) {
            emit dataChanged(oldIndex, oldIndex);
        }
        if (newIndex.isValid()) {
            emit dataChanged(newIndex, newIndex);
        }
    }
}

void AssetDBModel::reset(void)
{
    beginResetModel();
    childNodes_.clear();
    endResetModel();
}


QModelIndex AssetDBModel::index(int32_t row, int32_t column, const QModelIndex& parent) const
{
    QModelIndex result;

    if (!parent.isValid() && row == 0 && column == 0) { // session
        result = createIndex(0, 0, pRootNode_);
    }
	else if (parent.isValid() && column == 0)
    {
        FolderNode *parentNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
        Q_ASSERT(parentNode);
		auto it = childNodes_.constFind(parentNode);
        if (it == childNodes_.constEnd())
        {
            fetchMore(parentNode);
            it = childNodes_.constFind(parentNode);
        }

        if (row < it.value().size()) {
            result = createIndex(row, 0, it.value().at(row));
        }
    }

 //   qDebug() << "index of " << row << column << parent.data() << " is " << result.data();
    return result;
}

QModelIndex AssetDBModel::parent(const QModelIndex& idx) const
{
    QModelIndex parentIndex;
    if (Node* pNode = nodeForIndex(idx))
    {
        FolderNode* pParentNode = visibleFolderNode(pNode->parentFolderNode());
        if (pParentNode)
        {
            FolderNode* pGrandParentNode = visibleFolderNode(pParentNode->parentFolderNode());
            if (pGrandParentNode)
            {
                auto it = childNodes_.constFind(pGrandParentNode);
                if (it == childNodes_.constEnd())
                {
                    fetchMore(pGrandParentNode);
                    it = childNodes_.constFind(pGrandParentNode);
                }
                Q_ASSERT(it != childNodes_.constEnd());
                const int32_t row = it.value().indexOf(pParentNode);
                Q_ASSERT(row >= 0);
                parentIndex = createIndex(row, 0, pParentNode);
            } 
			else 
			{
                // top level node, parent is session
                parentIndex = index(0, 0, QModelIndex());
            }
        }
    }

 //   qDebug() << "parent of " << idx.data() << " is " << parentIndex.data();
    return parentIndex;
}



QVariant AssetDBModel::data(const QModelIndex& index, int32_t role) const
{
    QVariant result;
    if (Node* pNode = nodeForIndex(index))
    {
        switch (role)
        {
            case Qt::DisplayRole:
            {
                QString name = pNode->displayName();

                result = name;
                break;
            }
            case Qt::EditRole: {
                result = pNode->name();
                break;
            }
            case Qt::ToolTipRole: {
                result = pNode->tooltip();
                break;
            }
            case Qt::DecorationRole: {
				FolderNode* pFolderNode = qobject_cast<FolderNode*>(pNode);
                if (pFolderNode) {
                    bool expanded = false;
                    if(pTreeview_) {
                        expanded = pTreeview_->isExpanded(index);
                    }

                    result = pFolderNode->icon(expanded);
				}
				else {
					result = pNode->icon();
				}
                break;
            }
            case Qt::FontRole: {
                QFont font;
                if (pNode == pStartupProject_) {
                    font.setBold(true);
                }
                result = font;
                break;
            }
        }
    }

    return result;
}

Qt::ItemFlags AssetDBModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return 0;
    }

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (Node* pNode = nodeForIndex(index))
    {
        if (pNode == pRootNode_) {
            return 0; // no flags for session node...
        }

        if (!qobject_cast<ProjectNode*>(pNode))
        {
            // either folder or file node
            if (pNode->supportedActions(pNode).contains(ProjectAction::Rename)) {
                f = f | Qt::ItemIsEditable;
            }
        }
    }
    return f;
}

bool AssetDBModel::setData(const QModelIndex& index, const QVariant& value, int32_t role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);
    return false;
}

int AssetDBModel::rowCount(const QModelIndex& parent) const
{
	int32_t rows = 0;
    if (!parent.isValid()) {
        rows = 1;
    }
    else
    {
        FolderNode* pFolderNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
        if (pFolderNode && childNodes_.contains(pFolderNode)) {
            // if we have a cache node list for this folder.
            // we know how many children it has.
            rows = childNodes_.value(pFolderNode).size();
        }

        if(pFolderNode) {
        //    qDebug() << pFolderNode ->displayName() << " Rows" << rows;
        }
    }
    return rows;
}

int AssetDBModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool AssetDBModel::hasChildren(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return true;
    }

    FolderNode* pFolderNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
    if (!pFolderNode) {
        return false;
    }

    // if a folder node has lazy children we return true without loading them.
    if(pFolderNode->hasUnLoadedChildren()) {
        return true;
    }

    // only way to know if we have childen now is to get the children.
    // if this folder not in child cache, populate it.
    auto it = childNodes_.constFind(pFolderNode);
    if (it == childNodes_.constEnd()) {
        fetchMore(pFolderNode);
        it = childNodes_.constFind(pFolderNode);
    }

    // check if this node's cache is empty.
    return !it.value().isEmpty();
}


bool AssetDBModel::canFetchMore(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return false;
    }

	if (FolderNode* pFolderNode = qobject_cast<FolderNode*>(nodeForIndex(parent))) {
		// if the folder is not in the cache we can fetch more.
		return !childNodes_.contains(pFolderNode);
	}

	return false;
}

void AssetDBModel::fetchMore(const QModelIndex& parent)
{
    FolderNode* pFolderNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
    Q_ASSERT(pFolderNode);

   // beginInsertRows is called in added();

    fetchMore(pFolderNode);
}


void AssetDBModel::fetchMore(FolderNode* pFolderNode) const
{
    BUG_CHECK(pFolderNode);
    BUG_CHECK(!childNodes_.contains(pFolderNode));

    if(pFolderNode->hasUnLoadedChildren()) {
        pFolderNode->loadChildren();
    }

    // this gets all the folders child nodes and places then in a cache.
    QList<Node*> nodeList = childNodes(pFolderNode);
    childNodes_.insert(pFolderNode, nodeList);
}


Node* AssetDBModel::nodeForIndex(const QModelIndex& index) const
{
    if (index.isValid()) {
        return (Node*)index.internalPointer();
    }
    return nullptr;
}

QModelIndex AssetDBModel::indexForNode(const Node* node_)
{
    // We assume that we are only called for nodes that are represented

    // we use non-const pointers internally
    Node* pNode = const_cast<Node*>(node_);
    if (!pNode) {
        return QModelIndex();
    }

    if (pNode == pRootNode_) {
        return createIndex(0, 0, pRootNode_);
    }

    FolderNode* pParentNode = visibleFolderNode(pNode->parentFolderNode());

    // Do we have the parent mapped?
    auto it = childNodes_.constFind(pParentNode);
    if (it == childNodes_.constEnd()) {
        fetchMore(pParentNode);
        it = childNodes_.constFind(pParentNode);
    }
    if (it != childNodes_.constEnd()) {
        const int row = it.value().indexOf(pNode);
        if (row != -1) {
            return createIndex(row, 0, pNode);
        }
    }
    return QModelIndex();
}


void AssetDBModel::added(FolderNode* pFolderNode, const QList<Node*>& newNodeList)
{
	QModelIndex parentIndex = indexForNode(pFolderNode);

	// Old  list
	auto it = childNodes_.constFind(pFolderNode);
	if (it == childNodes_.constEnd()) {
		beginInsertRows(parentIndex, 0, newNodeList.size() - 1);
		childNodes_.insert(pFolderNode, newNodeList);
		endInsertRows();
		return;
	}

	QList<Node*> oldNodeList = it.value();

	// Compare lists and emit signals, and modify childNodes_ on the fly
	auto oldIter = oldNodeList.constBegin();
	auto newIter = newNodeList.constBegin();

	Q_ASSERT(isSorted(oldNodeList));
	Q_ASSERT(isSorted(newNodeList));


	QSet<Node*> emptyDifference;
	emptyDifference = oldNodeList.toSet();
	emptyDifference.subtract(newNodeList.toSet());

	if (!emptyDifference.isEmpty()) 
	{
		// This should not happen...
		qDebug() << "FlatModel::added, old Node list should be subset of newNode list, found files in old list which were not part of new list";
		for(Node* pNode : emptyDifference) {
			qDebug() << pNode->name();
		}
		Q_ASSERT(false);
	}

	// optimization, check for old list is empty
	if (oldIter == oldNodeList.constEnd()) 
	{
		// New Node List is empty, nothing added which intrest us
		if (newIter == newNodeList.constEnd()) {
			return;
		}
		// So all we need to do is easy
		beginInsertRows(parentIndex, 0, newNodeList.size() - 1);
		childNodes_.insert(pFolderNode, newNodeList);
		endInsertRows();
		return;
	}

	while (true) 
	{
		// Skip all that are the same
		while (*oldIter == *newIter) 
		{
			++oldIter;
			++newIter;
			if (oldIter == oldNodeList.constEnd())
			{
				// At end of oldNodeList, sweep up rest of newNodeList
				QList<Node *>::const_iterator startOfBlock = newIter;
				newIter = newNodeList.constEnd();
				int32_t pos = oldIter - oldNodeList.constBegin();
				int32_t count = newIter - startOfBlock;
				if (count > 0) 
				{
					beginInsertRows(parentIndex, pos, pos + count - 1);
						while (startOfBlock != newIter) 
						{
							oldNodeList.insert(pos, *startOfBlock);
							++pos;
							++startOfBlock;
						}
						childNodes_.insert(pFolderNode, oldNodeList);
					endInsertRows();
				}
				return; // Done with the lists, leave the function
			}
		}

		QList<Node *>::const_iterator startOfBlock = newIter;
		while (*oldIter != *newIter) {
			++newIter;
		}

		// startOfBlock is the first that was diffrent
		// newIter points to the new position of oldIter
		// newIter - startOfBlock is number of new items
		// oldIter is the position where those are...
		int32_t pos = oldIter - oldNodeList.constBegin();
		int32_t count = newIter - startOfBlock;

		beginInsertRows(parentIndex, pos, pos + count - 1);
			while (startOfBlock != newIter) 
			{
				oldNodeList.insert(pos, *startOfBlock);
				++pos;
				++startOfBlock;
			}
			childNodes_.insert(pFolderNode, oldNodeList);
		endInsertRows();

		oldIter = oldNodeList.constBegin() + pos;
	}
}

void AssetDBModel::removed(FolderNode* pParentNode, const QList<Node*>& newNodeList)
{
	X_UNUSED(pParentNode);
	X_UNUSED(newNodeList);
	BUG_ASSERT_NOT_IMPLEMENTED();
}

void AssetDBModel::removeFromCache(QList<FolderNode*> list)
{
	for (FolderNode* pFolderNode : list) {
		removeFromCache(pFolderNode->subFolderNodes());
		childNodes_.remove(pFolderNode);
	}
}

void AssetDBModel::changedSortKey(FolderNode* pFolderNode, Node* pNode)
{
	X_UNUSED(pFolderNode);
	X_UNUSED(pNode);
	BUG_ASSERT_NOT_IMPLEMENTED();
}


FolderNode* AssetDBModel::visibleFolderNode(FolderNode* pNode) const
{
    if (!pNode) {
        return nullptr;
    }

    // search parents till we get a visible folder node that's not filterd.
    for (FolderNode* pFolderNode = pNode; pFolderNode; pFolderNode = pFolderNode->parentFolderNode())
    {
        if (!filter(pFolderNode)) {
            return pFolderNode;
        }
    }
    return nullptr;
}

void AssetDBModel::recursiveAddFolderNodes(FolderNode* pStartNode, QList<Node*>* pList) const
{
	for (FolderNode* pFolderNode : pStartNode->subFolderNodes()) {
        if (pFolderNode) {
            recursiveAddFolderNodesImpl(pFolderNode, pList);
        }
    }
}

void AssetDBModel::recursiveAddFolderNodesImpl(FolderNode* pStartNode, QList<Node*>* pList) const
{
    if (!filter(pStartNode))
    {
        pList->append(pStartNode);
    }
    else
    {
        for (FolderNode* pFolderNode : pStartNode->subFolderNodes()) {
            if (pFolderNode) {
                recursiveAddFolderNodesImpl(pFolderNode, pList);
            }
        }
    }
}

void AssetDBModel::recursiveAddFileNodes(FolderNode* pStartNode, QList<Node*>* pList) const
{
	for (FolderNode* pSubFolderNode : pStartNode->subFolderNodes()) {
        recursiveAddFileNodes(pSubFolderNode, pList);
    }
	for (Node* pNode : pStartNode->fileNodes()) {
        if (!filter(pNode)) {
            pList->append(pNode);
        }
    }
}

void AssetDBModel::addFileNodes(FolderNode* pStartNode, QList<Node*>* pList) const
{
	for (Node* pNode : pStartNode->fileNodes()) {
		if (!filter(pNode)) {
			pList->append(pNode);
		}
	}
}

QList<Node*> AssetDBModel::childNodes(FolderNode *parentNode) const
{
    QList<Node*> nodeList;

    if (parentNode->nodeType() == NodeType::SessionNodeType)
    {
        SessionNode* pSessionNode = static_cast<SessionNode*>(parentNode);
        QList<ProjectNode*> projectList = pSessionNode->projectNodes();
        for (int32_t i = 0; i < projectList.size(); ++i) {
            nodeList.append(projectList.at(i));
        }
    }
    else
    {
        recursiveAddFolderNodes(parentNode, &nodeList);
		// can't use this since it will add folder below.
		// this is only useful if we are filtering out hidden nodes.
	//	recursiveAddFileNodes(parentNode, &nodeList);
		addFileNodes(parentNode, &nodeList);
    }

    qSort(nodeList.begin(), nodeList.end(), sortNodes);
    return nodeList;
}

X_INLINE bool AssetDBModel::filter(Node *node) const
{
    Q_UNUSED(node);

    bool isHidden = false;

#if 0 // nothings hidden currently.
    if (node->nodeType() == NodeType::SessionNodeType) {
        isHidden = false;
    } else if (ProjectNode *projectNode = qobject_cast<ProjectNode*>(node)) {

    } else if (node->nodeType() == NodeType::FolderNodeType || node->nodeType() == NodeType::VirtualFolderNodeType) {

    } else if (FileNode *fileNode = qobject_cast<FileNode*>(node)) {

    }
#endif

    return isHidden;
}


// 

void AssetDBModel::filesAboutToBeAdded(FolderNode* pFolder, const QList<FileNode*>& newFiles)
{
	Q_UNUSED(newFiles)
	pParentFolderForChange_ = pFolder;
}


void AssetDBModel::filesAdded(void)
{
	// First find out what the folder is that we are adding the files to
	FolderNode* pFolderNode = visibleFolderNode(pParentFolderForChange_);

	// Now get the new List for that folder
	QList<Node *> newNodeList = childNodes(pFolderNode);

	added(pFolderNode, newNodeList);
}

void AssetDBModel::foldersAboutToBeAdded(FolderNode* pParentFolder, const QList<FolderNode*>& newFolders)
{
	Q_UNUSED(newFolders)
	pParentFolderForChange_ = pParentFolder;
}

void AssetDBModel::foldersAdded(void)
{
	// First found out what the folder is that we are adding the files to
	FolderNode* pFolderNode = visibleFolderNode(pParentFolderForChange_);

	// Now get the new list for that folder
	QList<Node*> newNodeList = childNodes(pFolderNode);

	added(pFolderNode, newNodeList);
}

} // namespace AssetExplorer

X_NAMESPACE_END
