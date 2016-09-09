#include "assetdbnodes.h"
#include "assert_qt.h"

#include <QFileInfo>
#include <QDir>
#include <QIcon>
#include <QStyle>

namespace AssetExplorer
{


Node::Node(NodeType nodeType,
           const QString &name, int line) :
    QObject(),
    nodeType_(nodeType),
    projectNode_(0),
    folderNode_(0),
    name_(name),
    line_(line)
{

}

void Node::emitNodeSortKeyAboutToChange()
{
    if (ProjectNode *project = projectNode()) {
        foreach (NodesWatcher *watcher, project->watchers()) {
            emit watcher->nodeSortKeyAboutToChange(this);
        }
    }
}

void Node::emitNodeSortKeyChanged()
{
    if (ProjectNode *project = projectNode()) {
        foreach (NodesWatcher *watcher, project->watchers()) {
            emit watcher->nodeSortKeyChanged();
        }
    }
}

/*!
 * The path of the file representing this node.
 *
 * This function does not emit any signals. That has to be done by the calling
 * class.
 */
void Node::setName(const QString &name)
{
    if (name_ == name) {
        return;
    }

    emitNodeSortKeyAboutToChange();
    name_ = name;
    emitNodeSortKeyChanged();
    emitNodeUpdated();
}

void Node::setLine(int line)
{
    if (line_ == line) {
        return;
    }

    emitNodeSortKeyAboutToChange();
    line_ = line;
    emitNodeSortKeyChanged();
    emitNodeUpdated();
}

void Node::setNameAndLine(const QString &name, int line)
{
    if (name_ == name && line_ == line) {
        return;
    }

    emitNodeSortKeyAboutToChange();
    name_ = name;
    line_ = line;
    emitNodeSortKeyChanged();
    emitNodeUpdated();
}

NodeType Node::nodeType() const
{
    return nodeType_;
}

/*!
  The project that owns and manages the node. It is the first project in the list
  of ancestors.
  */
ProjectNode *Node::projectNode() const
{
    return projectNode_;
}

/*!
  The parent in the node hierarchy.
  */
FolderNode *Node::parentFolderNode() const
{
    return folderNode_;
}

QString Node::name() const
{
    return name_;
}

int Node::line() const
{
    return -1;
}

QString Node::displayName() const
{
    return name();
}

QString Node::tooltip() const
{
    return name();
}

bool Node::isEnabled() const
{
    return parentFolderNode()->isEnabled();
}

QList<ProjectAction> Node::supportedActions(Node *node) const
{
    QList<ProjectAction> list = parentFolderNode()->supportedActions(node);
    list.append(ProjectAction::InheritedFromParent);
    return list;
}

void Node::setNodeType(NodeType type)
{
    nodeType_ = type;
}

void Node::setProjectNode(ProjectNode *project)
{
    projectNode_ = project;
}

void Node::emitNodeUpdated()
{
    if (ProjectNode *node = projectNode()) {
        foreach (NodesWatcher *watcher, node->watchers()) {
            emit watcher->nodeUpdated(this);
        }
    }
}


void Node::setParentFolderNode(FolderNode *parentFolder)
{
    folderNode_ = parentFolder;
}



FileNode::FileNode(const QString &name,
                   const FileType fileType,
                   bool generated, int line) :
    Node(NodeType::FileNodeType, name, line),
    fileType_(fileType),
    generated_(generated)
{
}

FileType FileNode::fileType() const
{
    return fileType_;
}


bool FileNode::isGenerated() const
{
    return generated_;
}

FolderNode::FolderNode(const QString &name, NodeType nodeType)  :
    Node(nodeType, name),
    displayName_(QDir::toNativeSeparators(name))
{
}

FolderNode::~FolderNode()
{
    qDeleteAll(subFolderNodes_);
    qDeleteAll(fileNodes_);
}


QString FolderNode::displayName() const
{
    return displayName_;
}


QIcon FolderNode::icon(bool expanded) const
{
    if(expanded && !iconexpanded_.isNull()) {
        return iconexpanded_;
    }
    return icon_;
}

QList<FileNode*> FolderNode::fileNodes() const
{
    return fileNodes_;
}

QList<FolderNode*> FolderNode::subFolderNodes() const
{
    return subFolderNodes_;
}


void FolderNode::setDisplayName(const QString &name)
{
    if (displayName_ == name) {
        return;
    }

    emitNodeSortKeyAboutToChange();
    displayName_ = name;
    emitNodeSortKeyChanged();
    emitNodeUpdated();
}

void FolderNode::setIcon(const QIcon &icon)
{
    icon_ = icon;
}

void FolderNode::setIconExpanded(const QIcon &icon)
{
    iconexpanded_ = icon;
}


FileNode *FolderNode::findFile(const QString &name)
{
    foreach (FileNode *n, fileNodes()) {
        if (n->name() == name) {
            return n;
        }
    }
    return 0;
}

FolderNode *FolderNode::findSubFolder(const QString &name)
{
    foreach (FolderNode *n, subFolderNodes()) {
        if (n->name() == name) {
            return n;
        }
    }
    return 0;
}

bool FolderNode::hasLazyChildren(void) const
{
    return false;
}


bool FolderNode::preFetch()
{
    return true;
}


bool FolderNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    if (projectNode())
        return projectNode()->addFiles(filePaths, notAdded);
    return false;
}

bool FolderNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    if (projectNode())
        return projectNode()->removeFiles(filePaths, notRemoved);
    return false;
}

bool FolderNode::deleteFiles(const QStringList &filePaths)
{
    if (projectNode())
        return projectNode()->deleteFiles(filePaths);
    return false;
}

bool FolderNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    if (projectNode())
        return projectNode()->renameFile(filePath, newFilePath);
    return false;
}


void FolderNode::addFileNodes(const QList<FileNode *> &files)
{
    Q_ASSERT(projectNode());
    ProjectNode *pn = projectNode();

    if (files.isEmpty()) {
        return;
    }

    foreach (NodesWatcher *watcher, pn->watchers()) {
        emit watcher->filesAboutToBeAdded(this, files);
    }

    foreach (FileNode *file, files)
    {
        BUG_ASSERT(!file->parentFolderNode(),
                   qDebug("File node has already a parent folder"));

        file->setParentFolderNode(this);
        file->setProjectNode(pn);

        // Now find the correct place to insert file
        if (fileNodes_.count() == 0 || fileNodes_.last() < file)
        {
            // empty list or greater then last node
            fileNodes_.append(file);
        }
        else
        {
            auto it = qLowerBound(fileNodes_.begin(),
                                  fileNodes_.end(),
                                  file);

            fileNodes_.insert(it, file);
        }
    }

    foreach (NodesWatcher *watcher, pn->watchers()) {
        emit watcher->filesAdded();
    }
}


void FolderNode::removeFileNodes(const QList<FileNode *> &files)
{
    Q_ASSERT(projectNode());
    ProjectNode *pn = projectNode();

    if (files.isEmpty()) {
        return;
    }

    QList<FileNode*> toRemove = files;
    qSort(toRemove.begin(), toRemove.end());

    foreach (NodesWatcher *watcher, pn->watchers()) {
        emit watcher->filesAboutToBeRemoved(this, toRemove);
    }

    QList<FileNode*>::const_iterator toRemoveIter = toRemove.constBegin();
    QList<FileNode*>::iterator filesIter = fileNodes_.begin();
    for (; toRemoveIter != toRemove.constEnd(); ++toRemoveIter)
    {
        while (*filesIter != *toRemoveIter)
        {
            ++filesIter;

            BUG_ASSERT(filesIter != fileNodes_.end(),
                       qDebug("File to remove is not part of specified folder!"));
        }

        delete *filesIter;
        filesIter = fileNodes_.erase(filesIter);
    }

    foreach (NodesWatcher *watcher, pn->watchers()) {
        emit watcher->filesRemoved();
    }
}

void FolderNode::addFolderNodes(const QList<FolderNode*> &subFolders)
{
    Q_ASSERT(projectNode());
    ProjectNode *pn = projectNode();

    if (subFolders.isEmpty()) {
        return;
    }

    foreach (NodesWatcher *watcher, pn->watchers()) {
        watcher->foldersAboutToBeAdded(this, subFolders);
    }

    foreach (FolderNode *folder, subFolders)
    {
        BUG_ASSERT(!folder->parentFolderNode(),
                   qDebug("Project node has already a parent folder"));
        folder->setParentFolderNode(this);
        folder->setProjectNode(pn);

        // Find the correct place to insert
        if (subFolderNodes_.count() == 0 || subFolderNodes_.last() < folder)
        {
            // empty list or greater then last node
            subFolderNodes_.append(folder);
        }
        else
        {
            // Binary Search for insertion point
            auto it = qLowerBound(subFolderNodes_.begin(),
                                  subFolderNodes_.end(),
                                  folder);

            subFolderNodes_.insert(it, folder);
        }

        // project nodes have to be added via addProjectNodes
        BUG_ASSERT(folder->nodeType() != NodeType::ProjectNodeType,
                   qDebug("project nodes have to be added via addProjectNodes"));
    }

    foreach (NodesWatcher *watcher, pn->watchers()) {
        emit watcher->foldersAdded();
    }
}


void FolderNode::removeFolderNodes(const QList<FolderNode*> &subFolders)
{
    Q_ASSERT(projectNode());
    ProjectNode *pn = projectNode();

    if (subFolders.isEmpty()) {
        return;
    }

    QList<FolderNode*> toRemove = subFolders;
    qSort(toRemove.begin(), toRemove.end());

    foreach (NodesWatcher *watcher, pn->watchers()) {
        emit watcher->foldersAboutToBeRemoved(this, toRemove);
    }

    QList<FolderNode*>::const_iterator toRemoveIter = toRemove.constBegin();
    QList<FolderNode*>::iterator folderIter = subFolderNodes_.begin();
    for (; toRemoveIter != toRemove.constEnd(); ++toRemoveIter)
    {
        BUG_ASSERT((*toRemoveIter)->nodeType() != NodeType::ProjectNodeType,
                   qDebug("project nodes have to be removed via removeProjectNodes"));

        while (*folderIter != *toRemoveIter)
        {
            ++folderIter;

            BUG_ASSERT(folderIter != subFolderNodes_.end(),
                       qDebug("Folder to remove is not part of specified folder!"));
        }

        delete *folderIter;
        folderIter = subFolderNodes_.erase(folderIter);
    }

    foreach (NodesWatcher *watcher, pn->watchers()) {
        emit watcher->foldersRemoved();
    }
}


VirtualFolderNode::VirtualFolderNode(const QString &name, int priority)
    : FolderNode(name, NodeType::VirtualFolderNodeType),
      priority_(priority)
{
}

VirtualFolderNode::~VirtualFolderNode()
{
}

int VirtualFolderNode::priority() const
{
    return priority_;
}


ProjectNode::ProjectNode(const QString &projectFilePath)
        : FolderNode(projectFilePath, NodeType::ProjectNodeType)
{
    // project node "manages" itself
    setProjectNode(this);
    setDisplayName(QFileInfo(projectFilePath).fileName());

    setObjectName("projectnode");

}


QList<ProjectNode*> ProjectNode::subProjectNodes() const
{
    return subProjectNodes_;
}


QList<NodesWatcher*> ProjectNode::watchers() const
{
    return watchers_;
}


void ProjectNode::registerWatcher(NodesWatcher *watcher)
{
    if (!watcher) {
        return;
    }

    connect(watcher, SIGNAL(destroyed(QObject*)),
            this, SLOT(watcherDestroyed(QObject*)));

    watchers_.append(watcher);

    foreach (ProjectNode *subProject, subProjectNodes_) {
        subProject->registerWatcher(watcher);
    }
}

void ProjectNode::unregisterWatcher(NodesWatcher *watcher)
{
    if (!watcher) {
        return;
    }

    watchers_.removeOne(watcher);

    foreach (ProjectNode *subProject, subProjectNodes_) {
        subProject->unregisterWatcher(watcher);
    }
}


void ProjectNode::addProjectNodes(const QList<ProjectNode*> &subProjects)
{
    if (!subProjects.isEmpty())
    {
        QList<FolderNode*> folderNodes;
        foreach (ProjectNode *projectNode, subProjects) {
            folderNodes << projectNode;
        }

        foreach (NodesWatcher *watcher, watchers_) {
            emit watcher->foldersAboutToBeAdded(this, folderNodes);
        }

        foreach (ProjectNode *project, subProjects)
        {
            BUG_ASSERT(!project->parentFolderNode() || project->parentFolderNode() == this,
                       qDebug("Project node has already a parent"));

            project->setParentFolderNode(this);

            foreach (NodesWatcher *watcher, watchers_) {
                project->registerWatcher(watcher);
            }

            subFolderNodes_.append(project);
            subProjectNodes_.append(project);
        }

        qSort(subFolderNodes_.begin(), subFolderNodes_.end());
        qSort(subProjectNodes_.begin(), subProjectNodes_.end());

        foreach (NodesWatcher *watcher, watchers_) {
            emit watcher->foldersAdded();
        }
    }
}


void ProjectNode::removeProjectNodes(const QList<ProjectNode*> &subProjects)
{
    if (!subProjects.isEmpty())
    {
        QList<FolderNode*> toRemove;
        foreach (ProjectNode *projectNode, subProjects) {
            toRemove << projectNode;
        }

        qSort(toRemove.begin(), toRemove.end());

        foreach (NodesWatcher *watcher, watchers_) {
            emit watcher->foldersAboutToBeRemoved(this, toRemove);
        }

        QList<FolderNode*>::const_iterator toRemoveIter = toRemove.constBegin();
        QList<FolderNode*>::iterator folderIter = subFolderNodes_.begin();
        QList<ProjectNode*>::iterator projectIter = subProjectNodes_.begin();
        for (; toRemoveIter != toRemove.constEnd(); ++toRemoveIter)
        {
            while (*projectIter != *toRemoveIter)
            {
                ++projectIter;
                BUG_ASSERT(projectIter != subProjectNodes_.end(),
                    qDebug("Project to remove is not part of specified folder!"));
            }

            while (*folderIter != *toRemoveIter)
            {
                ++folderIter;
                BUG_ASSERT(folderIter != subFolderNodes_.end(),
                    qDebug("Project to remove is not part of specified folder!"));
            }

            delete *projectIter;
            projectIter = subProjectNodes_.erase(projectIter);
            folderIter = subFolderNodes_.erase(folderIter);
        }

        foreach (NodesWatcher *watcher, watchers_) {
            emit watcher->foldersRemoved();
        }
    }
}

void ProjectNode::watcherDestroyed(QObject *watcher)
{
    // cannot use qobject_cast here
    unregisterWatcher(static_cast<NodesWatcher*>(watcher));
}


SessionNode::SessionNode(QObject *parentObject)
    : FolderNode(QLatin1String("session"), NodeType::SessionNodeType)
{
    setParent(parentObject);
}

QList<ProjectAction> SessionNode::supportedActions(Node *node) const
{
    Q_UNUSED(node)
    return QList<ProjectAction>();
}

QList<NodesWatcher*> SessionNode::watchers() const
{
    return watchers_;
}

void SessionNode::registerWatcher(NodesWatcher *watcher)
{
    if (!watcher) {
        return;
    }
    connect(watcher, SIGNAL(destroyed(QObject*)),
            this, SLOT(watcherDestroyed(QObject*)));

    watchers_.append(watcher);

    foreach (ProjectNode *project, projectNodes_) {
        project->registerWatcher(watcher);
    }
}


void SessionNode::unregisterWatcher(NodesWatcher *watcher)
{
    if (!watcher) {
        return;
    }

    watchers_.removeOne(watcher);

    foreach (ProjectNode *project, projectNodes_) {
        project->unregisterWatcher(watcher);
    }
}

QList<ProjectNode*> SessionNode::projectNodes() const
{
    return projectNodes_;
}

void SessionNode::addProjectNodes(const QList<ProjectNode*> &projectNodes)
{
    if (!projectNodes.isEmpty())
    {
        QList<FolderNode*> folderNodes;
        foreach (ProjectNode *projectNode, projectNodes) {
            folderNodes << projectNode;
        }

        foreach (NodesWatcher *watcher, watchers_) {
            emit watcher->foldersAboutToBeAdded(this, folderNodes);
        }

        foreach (ProjectNode *project, projectNodes)
        {
            BUG_ASSERT(!project->parentFolderNode(),
                qDebug("Project node has already a parent folder"));

            project->setParentFolderNode(this);

            foreach (NodesWatcher *watcher, watchers_) {
                project->registerWatcher(watcher);
            }
            subFolderNodes_.append(project);
            projectNodes_.append(project);
        }

        qSort(subFolderNodes_);
        qSort(projectNodes_);

        foreach (NodesWatcher *watcher, watchers_) {
            emit watcher->foldersAdded();
        }
   }
}

void SessionNode::removeProjectNodes(const QList<ProjectNode*> &projectNodes)
{
    if (!projectNodes.isEmpty())
    {
        QList<FolderNode*> toRemove;
        foreach (ProjectNode *projectNode, projectNodes) {
            toRemove << projectNode;
        }

        qSort(toRemove);

        foreach (NodesWatcher *watcher, watchers_) {
            emit watcher->foldersAboutToBeRemoved(this, toRemove);
        }

        QList<FolderNode*>::const_iterator toRemoveIter = toRemove.constBegin();
        QList<FolderNode*>::iterator folderIter = subFolderNodes_.begin();
        QList<ProjectNode*>::iterator projectIter = projectNodes_.begin();
        for (; toRemoveIter != toRemove.constEnd(); ++toRemoveIter)
        {
            while (*projectIter != *toRemoveIter)
            {
                ++projectIter;
                BUG_ASSERT(projectIter != projectNodes_.end(),
                    qDebug("Project to remove is not part of specified folder!"));
            }

            while (*folderIter != *toRemoveIter)
            {
                ++folderIter;
                BUG_ASSERT(folderIter != subFolderNodes_.end(),
                    qDebug("Project to remove is not part of specified folder!"));
            }
            projectIter = projectNodes_.erase(projectIter);
            folderIter = subFolderNodes_.erase(folderIter);
        }

        foreach (NodesWatcher *watcher, watchers_) {
            emit watcher->foldersRemoved();
        }
    }
}

void SessionNode::watcherDestroyed(QObject *watcher)
{
    // cannot use qobject_cast here
    unregisterWatcher(static_cast<NodesWatcher*>(watcher));
}


NodesWatcher::NodesWatcher(QObject *parent)
        : QObject(parent)
{
}




} // namespace AssetExplorer
