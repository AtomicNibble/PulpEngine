#ifndef ASSETDBNODES_H
#define ASSETDBNODES_H

#include <QIcon>
#include <QObject>
#include <QStringList>
#include <QDebug>

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
    ProjectNode *projectNode(void) const;     // managing project
    FolderNode *parentFolderNode(void) const; // parent folder or project
    QString path(void) const;                 // file system path
    int line(void) const;
    virtual QString displayName(void) const;
    virtual QString tooltip(void) const;
    virtual bool isEnabled(void) const;

    virtual QList<ProjectAction> supportedActions(Node *node) const;

    void setPath(const QString &path);
    void setLine(int line);
    void setPathAndLine(const QString &path, int line);
    void emitNodeUpdated(void);

protected:
    Node(NodeType nodeType, const QString &path, int line = -1);

    void setNodeType(NodeType type);
    void setProjectNode(ProjectNode *project);
    void setParentFolderNode(FolderNode *parentFolder);

    void emitNodeSortKeyAboutToChange(void);
    void emitNodeSortKeyChanged(void);

private:
    NodeType nodeType_;
    ProjectNode *projectNode_;
    FolderNode *folderNode_;
    QString path_;
    int line_;
};



class FileNode : public Node
{
    Q_OBJECT
public:
    FileNode(const QString &filePath, const FileType fileType, bool generated, int line = -1);

    FileType fileType(void) const;
    bool isGenerated(void) const;

private:
    // managed by ProjectNode
    friend class FolderNode;
    friend class ProjectNode;

    FileType fileType_;
    bool generated_;
};


class FolderNode : public Node
{
    Q_OBJECT
public:
    explicit FolderNode(const QString &folderPath, NodeType nodeType = NodeType::FolderNodeType);
    virtual ~FolderNode();

    QString displayName(void) const;
    QIcon icon(bool expanded) const;

    QList<FileNode*> fileNodes(void) const;
    QList<FolderNode*> subFolderNodes(void) const;

    void setDisplayName(const QString &name);
    void setIcon(const QIcon &icon);
    void setIconExpanded(const QIcon &icon);

    FileNode *findFile(const QString &path);
    FolderNode *findSubFolder(const QString &path);

    virtual bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0);
    virtual bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0);
    virtual bool deleteFiles(const QStringList &filePaths);
    virtual bool renameFile(const QString &filePath, const QString &newFilePath);

    class AddNewInformation
    {
    public:
        AddNewInformation(const QString &name, int p)
            :displayName(name), priority(p)
        {}
        QString displayName;
        int priority;
    };

    virtual AddNewInformation addNewInformation(const QStringList &files) const;

    void addFileNodes(const QList<FileNode*> &files);
    void removeFileNodes(const QList<FileNode*> &files);

    void addFolderNodes(const QList<FolderNode*> &subFolders);
    void removeFolderNodes(const QList<FolderNode*> &subFolders);

protected:
    QList<FolderNode*> subFolderNodes_;
    QList<FileNode*> fileNodes_;

private:
    // managed by ProjectNode
    friend class ProjectNode;
    QString displayName_;
    mutable QIcon icon_;
    mutable QIcon iconexpanded_;
};


class VirtualFolderNode : public FolderNode
{
    Q_OBJECT
public:
    explicit VirtualFolderNode(const QString &folderPath, int priority);
    virtual ~VirtualFolderNode();

    int priority(void) const;
private:
    int priority_;
};



class ProjectNode : public FolderNode
{
    Q_OBJECT

public:
    QString vcsTopic() const;

    // all subFolders that are projects
    QList<ProjectNode*> subProjectNodes() const;

    // determines if the project will be shown in the flat view
    // TODO find a better name
    void aboutToChangeHasBuildTargets();
    void hasBuildTargetsChanged();
    virtual bool hasBuildTargets() const = 0;

    virtual bool canAddSubProject(const QString &proFilePath) const = 0;

    virtual bool addSubProjects(const QStringList &proFilePaths) = 0;

    virtual bool removeSubProjects(const QStringList &proFilePaths) = 0;

    // by default returns false
    virtual bool deploysFolder(const QString &folder) const;

    // TODO node parameter not really needed
    //virtual QList<ProjectExplorer::RunConfiguration *> runConfigurationsFor(Node *node) = 0;


    QList<NodesWatcher*> watchers(void) const;
    void registerWatcher(NodesWatcher *watcher);
    void unregisterWatcher(NodesWatcher *watcher);

    bool isEnabled(void) const { return true; }

    // to be called in implementation of
    // the corresponding public functions
    void addProjectNodes(const QList<ProjectNode*> &subProjects);
    void removeProjectNodes(const QList<ProjectNode*> &subProjects);

protected:
    // this is just the in-memory representation, a subclass
    // will add the persistent stuff
    explicit ProjectNode(const QString &projectFilePath);

private slots:
    void watcherDestroyed(QObject *watcher);

private:
    QList<ProjectNode*> subProjectNodes_;
    QList<NodesWatcher*> watchers_;

    // let SessionNode call setParentFolderNode
    friend class SessionNode;
};


class SessionNode : public FolderNode {
    Q_OBJECT
    friend class SessionManager;
public:
    SessionNode(QObject *parentObject);

    QList<ProjectAction> supportedActions(Node *node) const;

    QList<ProjectNode*> projectNodes() const;

    QList<NodesWatcher*> watchers() const;
    void registerWatcher(NodesWatcher *watcher);
    void unregisterWatcher(NodesWatcher *watcher);

    bool isEnabled(void) const { return true; }

protected:
    void addProjectNodes(const QList<ProjectNode*> &projectNodes);
    void removeProjectNodes(const QList<ProjectNode*> &projectNodes);

private slots:
    void watcherDestroyed(QObject *watcher);

private:
    QList<ProjectNode*> projectNodes_;
    QList<NodesWatcher*> watchers_;
};


class NodesWatcher : public QObject {
    Q_OBJECT
public:
    explicit NodesWatcher(QObject *parent = 0);

signals:
    // Emited whenever the model needs to send a update signal.
     void nodeUpdated(Node *node);

     // projects
     void aboutToChangeHasBuildTargets(ProjectNode* node);
     void hasBuildTargetsChanged(ProjectNode *node);

    // folders & projects
    void foldersAboutToBeAdded(FolderNode *parentFolder,
                               const QList<FolderNode*> &newFolders);
    void foldersAdded(void);

    void foldersAboutToBeRemoved(FolderNode *parentFolder,
                               const QList<FolderNode*> &staleFolders);
    void foldersRemoved(void);

    // files
    void filesAboutToBeAdded(FolderNode *folder,
                               const QList<FileNode*> &newFiles);
    void filesAdded(void);

    void filesAboutToBeRemoved(FolderNode *folder,
                               const QList<FileNode*> &staleFiles);
    void filesRemoved(void);
    void nodeSortKeyAboutToChange(Node *node);
    void nodeSortKeyChanged(void);

private:

    // let project & session emit signals
    friend class ProjectNode;
    friend class FolderNode;
    friend class SessionNode;
    friend class Node;
};

} // namespace AssetExplorer

#endif // ASSETDBNODES_H
