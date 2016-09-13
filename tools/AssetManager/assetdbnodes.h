#ifndef ASSETDBNODES_H
#define ASSETDBNODES_H


X_NAMESPACE_BEGIN(assman)

namespace AssetExplorer
{


enum class NodeType {
    FileNodeType = 1,
    FolderNodeType,
    VirtualFolderNodeType,
    ProjectNodeType,
    SessionNodeType
};

// File types common for projects
enum class FileType {
    UnknownFileType = 0,
    SourceType,
    ProjectFileType,
    FileTypeSize
};

enum class ProjectAction {
    // Special value to indicate that the actions are handled by the parent
    InheritedFromParent,
    AddSubProject,
    RemoveSubProject,
    // Let's the user select to which project file
    // the file is added
    AddNewFile,
    AddExistingFile,
    // Add files, which match user defined filters,
    // from an existing directory and its subdirectories
    AddExistingDirectory,
    // Removes a file from the project, optionally also
    // delete it on disc
    RemoveFile,
    // Deletes a file from the file system, informs the project
    // that a file was deleted
    // DeleteFile is a define on windows...
    EraseFile,
    Rename,
    // hides actions that use the path(): Open containing folder, open terminal here and Find in Directory
    HidePathActions,
    HasSubProjectRunConfigurations
};

class Node;
class FileNode;
class FileContainerNode;
class FolderNode;
class ProjectNode;
class NodesWatcher;
class NodesVisitor;
class SessionManager;



class Node : public QObject
{
    Q_OBJECT
public:
    NodeType nodeType(void) const;
    ProjectNode* projectNode(void) const;     // managing project
    FolderNode* parentFolderNode(void) const; // parent folder or project
    QString name(void) const;                 // name
    int32_t line(void) const;
	QIcon icon(void) const;
    virtual QString displayName(void) const;
    virtual QString tooltip(void) const;
    virtual bool isEnabled(void) const;

    virtual QList<ProjectAction> supportedActions(Node *node) const;

    void setName(const QString& name);
    void setLine(int32_t line);
    void setNameAndLine(const QString& name, int32_t line);
	void setIcon(const QIcon& icon);
    void emitNodeUpdated(void);

protected:
    Node(NodeType nodeType, const QString& name, int32_t line = -1);

    void setNodeType(NodeType type);
    void setProjectNode(ProjectNode* pProject);
    void setParentFolderNode(FolderNode* pParentFolder);

    void emitNodeSortKeyAboutToChange(void);
    void emitNodeSortKeyChanged(void);

private:
    NodeType nodeType_;
    ProjectNode* pProjectNode_;
    FolderNode* pFolderNode_;
    QString name_;
	mutable QIcon icon_;
    int32_t line_;
};



class FileNode : public Node
{
    Q_OBJECT
public:
    FileNode(const QString& name, const FileType fileType, int32_t line = -1);

    FileType fileType(void) const;

private:
    // managed by ProjectNode
    friend class FolderNode;
    friend class ProjectNode;

    FileType fileType_;
};


class FolderNode : public Node
{
    Q_OBJECT
public:
    explicit FolderNode(const QString& name, NodeType nodeType = NodeType::FolderNodeType);
    virtual ~FolderNode();

    QString displayName(void) const override;
	QIcon icon(bool expanded) const;
	QIcon iconExpanded(void) const;

    QList<FileNode*> fileNodes(void) const;
    QList<FolderNode*> subFolderNodes(void) const;

	void setIconExpanded(const QIcon& icon);
    void setDisplayName(const QString& name);

    FileNode* findFile(const QString& path);
    FolderNode* findSubFolder(const QString& path);

    virtual bool hasUnLoadedChildren(void) const;
    virtual bool loadChildren(void);

    void addFileNodes(const QList<FileNode*>& files);
    void removeFileNodes(const QList<FileNode*>& files);

    void addFolderNodes(const QList<FolderNode*>& subFolders);
    void removeFolderNodes(const QList<FolderNode*>& subFolders);

protected:
    QList<FolderNode*> subFolderNodes_;
    QList<FileNode*> fileNodes_;

private:
    // managed by ProjectNode
    friend class ProjectNode;
    QString displayName_;
	mutable QIcon iconexpanded_;
};


class VirtualFolderNode : public FolderNode
{
    Q_OBJECT
public:
    explicit VirtualFolderNode(const QString& name, int32_t priority);
    virtual ~VirtualFolderNode(void);

	int32_t priority(void) const;
private:
	int32_t priority_;
};



class ProjectNode : public FolderNode
{
    Q_OBJECT

public:

    // all subFolders that are projects
    QList<ProjectNode*> subProjectNodes(void) const;

    // determines if the project will be shown in the flat view
    // TODO find a better name

    virtual bool canAddSubProject(const QString& proFilePath) const X_ABSTRACT;
    virtual bool addSubProjects(const QStringList& proFilePaths) X_ABSTRACT;
    virtual bool removeSubProjects(const QStringList& proFilePaths) X_ABSTRACT;


    QList<NodesWatcher*> watchers(void) const;
    void registerWatcher(NodesWatcher* pWatcher);
    void unregisterWatcher(NodesWatcher* pWatcher);

	bool isEnabled(void) const X_OVERRIDE;

    // to be called in implementation of
    // the corresponding public functions
    void addProjectNodes(const QList<ProjectNode*>& subProjects);
    void removeProjectNodes(const QList<ProjectNode*>& subProjects);

protected:
    // this is just the in-memory representation, a subclass
    // will add the persistent stuff
    explicit ProjectNode(const QString& projectFilePath);

private slots:
    void watcherDestroyed(QObject* pWatcher);

private:
    QList<ProjectNode*> subProjectNodes_;
    QList<NodesWatcher*> watchers_;

    // let SessionNode call setParentFolderNode
    friend class SessionNode;
};


class SessionNode : public FolderNode
{
    Q_OBJECT
    friend class SessionManager;
public:
    SessionNode(QObject *parentObject);

    QList<ProjectAction> supportedActions(Node *node) const;

    QList<ProjectNode*> projectNodes(void) const;

    QList<NodesWatcher*> watchers(void) const;
    void registerWatcher(NodesWatcher* pWatcher);
    void unregisterWatcher(NodesWatcher* pWatcher);

    bool isEnabled(void) const { return true; }

protected:
    void addProjectNodes(const QList<ProjectNode*>& projectNodes);
    void removeProjectNodes(const QList<ProjectNode*>& projectNodes);

private slots:
    void watcherDestroyed(QObject* pWatcher);

private:
    QList<ProjectNode*> projectNodes_;
    QList<NodesWatcher*> watchers_;
};


class NodesWatcher : public QObject
{
    Q_OBJECT
public:
    explicit NodesWatcher(QObject *parent = 0);

signals:
    // Emited whenever the model needs to send a update signal.
     void nodeUpdated(Node *node);

    // folders&  projects
    void foldersAboutToBeAdded(FolderNode *parentFolder,
                               const QList<FolderNode*>& newFolders);
    void foldersAdded(void);

    void foldersAboutToBeRemoved(FolderNode *parentFolder,
                               const QList<FolderNode*>& staleFolders);
    void foldersRemoved(void);

    // files
    void filesAboutToBeAdded(FolderNode *folder,
                               const QList<FileNode*>& newFiles);
    void filesAdded(void);

    void filesAboutToBeRemoved(FolderNode *folder,
                               const QList<FileNode*>& staleFiles);
    void filesRemoved(void);
    void nodeSortKeyAboutToChange(Node *node);
    void nodeSortKeyChanged(void);

private:

    // let project&  session emit signals
    friend class ProjectNode;
    friend class FolderNode;
    friend class SessionNode;
    friend class Node;
};

} // namespace AssetExplorer

X_NAMESPACE_END

#endif // ASSETDBNODES_H
