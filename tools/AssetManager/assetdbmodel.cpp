#include "assetdbmodel.h"
#include "assert.h"

#include "assetdbnodes.h"
#include "assetdb.h"

#include <QSqlQueryModel>
#include <QDebug>
#include <QTreeView>
#include <QFileInfo>
#include <QStringList>

namespace AssetExplorer
{

namespace {

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

    // project files
    FileNode *file1 = qobject_cast<FileNode*>(n1);
    FileNode *file2 = qobject_cast<FileNode*>(n2);
    if (file1 && file1->fileType() == FileType::ProjectFileType) {
        if (file2 && file2->fileType() == FileType::ProjectFileType) {
            const QString fileName1 = file1->name();
            const QString fileName2 = file2->name();

            int result = caseFriendlyCompare(fileName1, fileName2);
            if (result != 0)
                return result < 0;
            else
                return file1 < file2;
        } else {
            return true; // project file is before everything else
        }
    } else {
        if (file2 && file2->fileType() == FileType::ProjectFileType)
            return false;
    }

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

} // namnespace



AssetDBModel::AssetDBModel(SessionNode *rootNode, AssetDb* pDb, QObject *parent) :
    QAbstractItemModel(parent),
    treeview_(nullptr),
    startupProject_(nullptr),
    rootNode_(rootNode),
    db_(pDb)
{

}

AssetDBModel::~AssetDBModel()
{

}

void AssetDBModel::setTreeView(QTreeView* pTree)
{
    treeview_ = pTree;
}

void AssetDBModel::setStartupProject(ProjectNode *projectNode)
{
    if (startupProject_ != projectNode) {
        QModelIndex oldIndex = (startupProject_ ? indexForNode(startupProject_) : QModelIndex());
        QModelIndex newIndex = (projectNode ? indexForNode(projectNode) : QModelIndex());
        startupProject_ = projectNode;
        if (oldIndex.isValid()) {
            emit dataChanged(oldIndex, oldIndex);
        }
        if (newIndex.isValid()) {
            emit dataChanged(newIndex, newIndex);
        }
    }
}

void AssetDBModel::reset()
{
    beginResetModel();
    childNodes_.clear();
    endResetModel();
}


QModelIndex AssetDBModel::index(int row, int column, const QModelIndex &parent) const
{
    QModelIndex result;
    if (!parent.isValid() && row == 0 && column == 0) { // session
        result = createIndex(0, 0, rootNode_);
    } else if (parent.isValid() && column == 0) {
        FolderNode *parentNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
        Q_ASSERT(parentNode);
        QHash<FolderNode*, QList<Node*> >::const_iterator it = childNodes_.constFind(parentNode);
        if (it == childNodes_.constEnd()) {
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

QModelIndex AssetDBModel::parent(const QModelIndex &idx) const
{
    QModelIndex parentIndex;
    if (Node *node = nodeForIndex(idx)) {
        FolderNode *parentNode = visibleFolderNode(node->parentFolderNode());
        if (parentNode) {
            FolderNode *grandParentNode = visibleFolderNode(parentNode->parentFolderNode());
            if (grandParentNode) {
                QHash<FolderNode*, QList<Node*> >::const_iterator it = childNodes_.constFind(grandParentNode);
                if (it == childNodes_.constEnd()) {
                    fetchMore(grandParentNode);
                    it = childNodes_.constFind(grandParentNode);
                }
                Q_ASSERT(it != childNodes_.constEnd());
                const int row = it.value().indexOf(parentNode);
                Q_ASSERT(row >= 0);
                parentIndex = createIndex(row, 0, parentNode);
            } else {
                // top level node, parent is session
                parentIndex = index(0, 0, QModelIndex());
            }
        }
    }

 //   qDebug() << "parent of " << idx.data() << " is " << parentIndex.data();
    return parentIndex;
}



QVariant AssetDBModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    if (Node *node = nodeForIndex(index))
    {
        FolderNode *folderNode = qobject_cast<FolderNode*>(node);
        switch (role)
        {
            case Qt::DisplayRole:
            {
                QString name = node->displayName();



                result = name;
                break;
            }
            case Qt::EditRole: {
                result = node->name();
                break;
            }
            case Qt::ToolTipRole: {
                result = node->tooltip();
                break;
            }
            case Qt::DecorationRole: {
                if (folderNode) {
                    bool expanded = false;
                    if(treeview_) {
                        expanded = treeview_->isExpanded(index);
                    }

                    result = folderNode->icon(expanded);
                }
                break;
            }
            case Qt::FontRole: {
                QFont font;
                if (node == startupProject_) {
                    font.setBold(true);
                }
                result = font;
                break;
            }
        }
    }

    return result;
}

Qt::ItemFlags AssetDBModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return 0;
    }

    Qt::ItemFlags f = Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    if (Node *node = nodeForIndex(index)) {
         if (node == rootNode_)
             return 0; // no flags for session node...
         if (!qobject_cast<ProjectNode *>(node)) {
             // either folder or file node
             if (node->supportedActions(node).contains(ProjectAction::Rename)) {
                 f = f | Qt::ItemIsEditable;
             }
         }
     }
    return f;
}

bool AssetDBModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);
    return false;
}

int AssetDBModel::rowCount(const QModelIndex & parent) const
{
    int rows = 0;
     if (!parent.isValid()) {
         rows = 1;
     } else {
         FolderNode *folderNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
         if (folderNode && childNodes_.contains(folderNode)) {
             rows = childNodes_.value(folderNode).size();
           }

         if(folderNode) {
              qDebug() << folderNode->displayName() << " Rows" << rows;
         }
     }
     return rows;
}

int AssetDBModel::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool AssetDBModel::hasChildren(const QModelIndex & parent) const
{
    if (!parent.isValid()) {
         return true;
    }

     FolderNode *folderNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
     if (!folderNode) {
         return false;
     }

     QHash<FolderNode*, QList<Node*> >::const_iterator it = childNodes_.constFind(folderNode);
     if (it == childNodes_.constEnd()) {
         fetchMore(folderNode);
         it = childNodes_.constFind(folderNode);
     }

     return !it.value().isEmpty();
}


bool AssetDBModel::canFetchMore(const QModelIndex & parent) const
{
    if (!parent.isValid()) {
        return false;
    } else {
        if (FolderNode *folderNode = qobject_cast<FolderNode*>(nodeForIndex(parent)))
            return !childNodes_.contains(folderNode);
        else
            return false;
    }
}

void AssetDBModel::fetchMore(const QModelIndex &parent)
{
    FolderNode *folderNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
    Q_ASSERT(folderNode);

    fetchMore(folderNode);
}


void AssetDBModel::fetchMore(FolderNode *folderNode) const
{
    BUG_CHECK(folderNode);
    BUG_CHECK(!childNodes_.contains(folderNode));

    QList<Node*> nodeList = childNodes(folderNode);
    childNodes_.insert(folderNode, nodeList);
}


Node *AssetDBModel::nodeForIndex(const QModelIndex &index) const
{
    if (index.isValid()) {
        return (Node*)index.internalPointer();
    }
    return 0;
}

QModelIndex AssetDBModel::indexForNode(const Node *node_)
{
    // We assume that we are only called for nodes that are represented

    // we use non-const pointers internally
    Node *node = const_cast<Node*>(node_);
    if (!node) {
        return QModelIndex();
    }

    if (node == rootNode_) {
        return createIndex(0, 0, rootNode_);
    }

    FolderNode *parentNode = visibleFolderNode(node->parentFolderNode());

    // Do we have the parent mapped?
    QHash<FolderNode*, QList<Node*> >::const_iterator it = childNodes_.constFind(parentNode);
    if (it == childNodes_.constEnd()) {
        fetchMore(parentNode);
        it = childNodes_.constFind(parentNode);
    }
    if (it != childNodes_.constEnd()) {
        const int row = it.value().indexOf(node);
        if (row != -1) {
            return createIndex(row, 0, node);
        }
    }
    return QModelIndex();
}

FolderNode *AssetDBModel::visibleFolderNode(FolderNode *node) const
{
    if (!node)
        return 0;

    for (FolderNode *folderNode = node;
         folderNode;
         folderNode = folderNode->parentFolderNode()) {
        if (!filter(folderNode)) {
            return folderNode;
        }
    }
    return 0;
}

QList<Node*> AssetDBModel::childNodes(FolderNode *parentNode, const QSet<Node*> &blackList) const
{
    QList<Node*> nodeList;

    if (parentNode->nodeType() == NodeType::SessionNodeType) {
        SessionNode *sessionNode = static_cast<SessionNode*>(parentNode);
        QList<ProjectNode*> projectList = sessionNode->projectNodes();
        for (int i = 0; i < projectList.size(); ++i) {
            if (!blackList.contains(projectList.at(i)))
                nodeList << projectList.at(i);
        }
    } else {
 //       recursiveAddFolderNodes(parentNode, &nodeList, blackList);
 //       recursiveAddFileNodes(parentNode, &nodeList, blackList + nodeList.toSet());
    }
    qSort(nodeList.begin(), nodeList.end(), sortNodes);
    return nodeList;
}

bool AssetDBModel::filter(Node *node) const
{
    bool isHidden = false;
    if (node->nodeType() == NodeType::SessionNodeType) {
        isHidden = false;
    } else if (ProjectNode *projectNode = qobject_cast<ProjectNode*>(node)) {
        //if (m_filterProjects && projectNode->parentFolderNode() != m_rootNode)
        //    isHidden = !projectNode->hasBuildTargets();
    } else if (node->nodeType() == NodeType::FolderNodeType || node->nodeType() == NodeType::VirtualFolderNodeType) {
       // if (m_filterProjects)
       //     isHidden = true;
    } else if (FileNode *fileNode = qobject_cast<FileNode*>(node)) {
       // if (m_filterGeneratedFiles)
       //     isHidden = fileNode->isGenerated();
    }
    return isHidden;
}

} // namespace AssetExplorer

