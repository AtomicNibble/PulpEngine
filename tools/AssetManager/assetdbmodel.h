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
    AssetDBModel(SessionNode *rootNode, AssetDb& db, QObject *parent = nullptr);
    ~AssetDBModel();

    void setTreeView(QTreeView* pTree);

    void reset();
    void setStartupProject(ProjectNode *projectNode);


    QModelIndex index(int32_t row, int32_t column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant &value, int role) override;

    int32_t rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int32_t columnCount(const QModelIndex& parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;


    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;
    void fetchMore(FolderNode *foldernode) const;

    Node *nodeForIndex(const QModelIndex& index) const;
    QModelIndex indexForNode(const Node *node);

	void added(FolderNode* pParentNode, const QList<Node*>& newNodeList);
	void removed(FolderNode* pParentNode, const QList<Node*>& newNodeList);
	void removeFromCache(QList<FolderNode*> list);
	void changedSortKey(FolderNode* pFolderNode, Node* pNode);

    void recursiveAddFolderNodes(FolderNode* pStartNode, QList<Node*>* pList) const;
    void recursiveAddFolderNodesImpl(FolderNode* pStartNode, QList<Node*>* pList) const;
    void recursiveAddFileNodes(FolderNode* pStartNode, QList<Node*>* pList) const;
    QList<Node*> childNodes(FolderNode *parentNode) const;

    FolderNode* visibleFolderNode(FolderNode *node) const;
    bool filter(Node *node) const;


private slots:

	void foldersAboutToBeAdded(FolderNode* pParentFolder, const QList<FolderNode*>& newFolders);
	void foldersAdded(void);

	// files
	void filesAboutToBeAdded(FolderNode* pFolder, const QList<FileNode*>& newFiles);
	void filesAdded(void);


private:
   QTreeView* treeview_;
   ProjectNode* startupProject_;
   SessionNode* rootNode_;
   mutable QHash<FolderNode*, QList<Node*> > childNodes_;

   FolderNode* pParentFolderForChange_;

   AssetDb& db_;
};

} // namespace AssetExplorer


#endif // ASSETDB_MODEL_H
