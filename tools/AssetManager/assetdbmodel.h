#ifndef ASSETDB_MODEL_H
#define ASSETDB_MODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QList>
#include <QSet>
#include <QHash>

class QTreeView;

class AssetDb;

namespace AssetExplorer
{

class Node;
class FileNode;
class FolderNode;
class ProjectNode;
class SessionNode;


class AssetDBModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    AssetDBModel(SessionNode *rootNode, AssetDb* pDb, QObject *parent = nullptr);
    ~AssetDBModel();

    void setTreeView(QTreeView* pTree);

    void reset();
    void setStartupProject(ProjectNode *projectNode);


    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex & parent = QModelIndex()) const;



    void added(FolderNode* folderNode, const QList<Node*> &newNodeList);
    void removed(FolderNode* parentNode, const QList<Node*> &newNodeList);
    void removeFromCache(QList<FolderNode *> list);
    void changedSortKey(FolderNode *folderNode, Node *node);

    bool canFetchMore(const QModelIndex & parent) const;
    void fetchMore(const QModelIndex & parent);
    void fetchMore(FolderNode *foldernode) const;

    Node *nodeForIndex(const QModelIndex &index) const;
    QModelIndex indexForNode(const Node *node);

    QList<Node*> childNodes(FolderNode *parentNode, const QSet<Node*> &blackList = QSet<Node*>()) const;
    FolderNode *visibleFolderNode(FolderNode *node) const;
    bool filter(Node *node) const;

private:
   QTreeView* treeview_;
   ProjectNode* startupProject_;
   SessionNode* rootNode_;
   mutable QHash<FolderNode*, QList<Node*> > childNodes_;

   AssetDb* db_;
};

} // namespace AssetExplorer


#endif // ASSETDB_MODEL_H
