#include "assetdbnodes.h"
#include "assert_qt.h"

#include <QFileInfo>
#include <QDir>
#include <QIcon>
#include <QStyle>

X_NAMESPACE_BEGIN(editor)


namespace AssetExplorer
{


Node::Node(NodeType nodeType, const QString &name) :
    QObject(),
    nodeType_(nodeType),
    pProjectNode_(nullptr),
	pFolderNode_(nullptr),
    name_(name)
{

}

void Node::emitNodeSortKeyAboutToChange(void)
{
    if (ProjectNode* pProject = projectNode()) {
        foreach (NodesWatcher* pWatcher, pProject->watchers()) {
            emit pWatcher->nodeSortKeyAboutToChange(this);
        }
    }
}

void Node::emitNodeSortKeyChanged(void)
{
    if (ProjectNode* pProject = projectNode()) {
        foreach (NodesWatcher* pWatcher, pProject->watchers()) {
            emit pWatcher->nodeSortKeyChanged();
        }
    }
}

/*!
 * The path of the file representing this node.
 *
 * This function does not emit any signals. That has to be done by the calling
 * class.
 */
void Node::setName(const QString& name)
{
    if (name_ == name) {
        return;
    }

    emitNodeSortKeyAboutToChange();
    name_ = name;
    emitNodeSortKeyChanged();
    emitNodeUpdated();
}


NodeType Node::nodeType(void) const
{
    return nodeType_;
}

/*!
  The project that owns and manages the node. It is the first project in the list
  of ancestors.
  */
ProjectNode* Node::projectNode(void) const
{
    return pProjectNode_;
}

FolderNode* Node::parentFolderNode(void) const
{
    return pFolderNode_;
}

QString Node::name(void) const
{
    return name_;
}

QIcon Node::icon(void) const
{
	return icon_;
}

QString Node::displayName(void) const
{
	return QFileInfo(name()).fileName();
}

QString Node::tooltip(void) const
{
    return name();
}

bool Node::isEnabled(void) const
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

void Node::setProjectNode(ProjectNode* project)
{
    pProjectNode_ = project;
}

void Node::setIcon(const QIcon& icon)
{
	icon_ = icon;
}


void Node::emitNodeUpdated(void)
{
    if (ProjectNode *node = projectNode()) {
        foreach (NodesWatcher* pWatcher, node->watchers()) {
            emit pWatcher->nodeUpdated(this);
        }
    }
}


void Node::setParentFolderNode(FolderNode* parentFolder)
{
    pFolderNode_ = parentFolder;
}

// ------------------------------------------------------------------

FileNode::FileNode(const QString& name, const assetDb::AssetType::Enum assetType) :
    Node(NodeType::FileNodeType, name),
	assetType_(assetType)
{
}

assetDb::AssetType::Enum FileNode::assetType(void) const
{
    return assetType_;
}


FolderNode::FolderNode(const QString& name, NodeType nodeType)  :
    Node(nodeType, name),
    displayName_(QDir::toNativeSeparators(name))
{
}

FolderNode::~FolderNode()
{
    qDeleteAll(subFolderNodes_);
    qDeleteAll(fileNodes_);
}


QString FolderNode::displayName(void) const
{
    return displayName_;
}

QIcon FolderNode::icon(bool expanded) const
{
	if (expanded && !iconexpanded_.isNull()) {
		return iconexpanded_;
	}
	return Node::icon();
}

QIcon FolderNode::iconExpanded(void) const
{
	return iconexpanded_;
}

QList<FileNode*> FolderNode::fileNodes(void) const
{
    return fileNodes_;
}

QList<FolderNode*> FolderNode::subFolderNodes(void) const
{
    return subFolderNodes_;
}

void FolderNode::setIconExpanded(const QIcon& icon)
{
	iconexpanded_ = icon;
}

void FolderNode::setDisplayName(const QString& name)
{
    if (displayName_ == name) {
        return;
    }

    emitNodeSortKeyAboutToChange();
    displayName_ = name;
    emitNodeSortKeyChanged();
    emitNodeUpdated();
}

FileNode* FolderNode::findFile(const QString& name, assetDb::AssetType::Enum type)
{
    for (FileNode* pNode : fileNodes()) {
        if (pNode->assetType() == type && pNode->name() == name) {
            return pNode;
        }
    }
    return nullptr;
}

FolderNode* FolderNode::findSubFolder(const QString& name)
{
    for (FolderNode* pNode : subFolderNodes()) {
        if (pNode->name() == name) {
            return pNode;
        }
    }
    return nullptr;
}

bool FolderNode::hasUnLoadedChildren(void) const
{
    return false;
}


bool FolderNode::loadChildren(void)
{
    return true;
}


void FolderNode::addFileNodes(const QList<FileNode*>& files)
{
    Q_ASSERT(projectNode());
    ProjectNode* pProjectNode = projectNode();

    if (files.isEmpty()) {
        return;
    }

    for (NodesWatcher* pWatcher : pProjectNode->watchers()) {
        emit pWatcher->filesAboutToBeAdded(this, files);
    }

	for (FileNode* pFile : files)
    {
        BUG_ASSERT(!pFile->parentFolderNode(),
                   qDebug("File node has already a parent folder"));

		pFile->setParentFolderNode(this);
		pFile->setProjectNode(pProjectNode);

        // Now find the correct place to insert file
        if (fileNodes_.count() == 0 || fileNodes_.last() < pFile)
        {
            // empty list or greater then last node
            fileNodes_.append(pFile);
        }
        else
        {
            auto it = qLowerBound(fileNodes_.begin(),
                                  fileNodes_.end(),
								  pFile);

            fileNodes_.insert(it, pFile);
        }
    }

	for (NodesWatcher* pWatcher : pProjectNode->watchers()) {
        emit pWatcher->filesAdded();
    }
}


void FolderNode::removeFileNodes(const QList<FileNode*>& files)
{
    Q_ASSERT(projectNode());
    ProjectNode *pn = projectNode();

    if (files.isEmpty()) {
        return;
    }

    QList<FileNode*> toRemove = files;
    qSort(toRemove.begin(), toRemove.end());

    foreach (NodesWatcher* pWatcher, pn->watchers()) {
        emit pWatcher->filesAboutToBeRemoved(this, toRemove);
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

    foreach (NodesWatcher* pWatcher, pn->watchers()) {
        emit pWatcher->filesRemoved();
    }
}

void FolderNode::addFolderNodes(const QList<FolderNode*>& subFolders)
{
    Q_ASSERT(projectNode());
    ProjectNode* pProjectNode = projectNode();

    if (subFolders.isEmpty()) {
        return;
    }

    for (NodesWatcher* pWatcher : pProjectNode->watchers()) {
		pWatcher->foldersAboutToBeAdded(this, subFolders);
    }

    for (FolderNode* pFolder : subFolders)
    {
        BUG_ASSERT(!pFolder->parentFolderNode(),
                   qDebug("Project node has already a parent folder"));
		pFolder->setParentFolderNode(this);
		pFolder->setProjectNode(pProjectNode);

        // Find the correct place to insert
        if (subFolderNodes_.count() == 0 || subFolderNodes_.last() < pFolder)
        {
            // empty list or greater then last node
            subFolderNodes_.append(pFolder);
        }
        else
        {
            // Binary Search for insertion point
            auto it = qLowerBound(subFolderNodes_.begin(),
                                  subFolderNodes_.end(),
								  pFolder);

            subFolderNodes_.insert(it, pFolder);
        }

        // project nodes have to be added via addProjectNodes
        BUG_ASSERT(pFolder->nodeType() != NodeType::ProjectNodeType,
                   qDebug("project nodes have to be added via addProjectNodes"));
    }

    for (NodesWatcher* pWatcher : pProjectNode->watchers()) {
        emit pWatcher->foldersAdded();
    }
}


void FolderNode::removeFolderNodes(const QList<FolderNode*>& subFolders)
{
    Q_ASSERT(projectNode());
    ProjectNode *pn = projectNode();

    if (subFolders.isEmpty()) {
        return;
    }

    QList<FolderNode*> toRemove = subFolders;
    qSort(toRemove.begin(), toRemove.end());

    foreach (NodesWatcher* pWatcher, pn->watchers()) {
        emit pWatcher->foldersAboutToBeRemoved(this, toRemove);
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

    foreach (NodesWatcher* pWatcher, pn->watchers()) {
        emit pWatcher->foldersRemoved();
    }
}

bool FolderNode::addFile(const core::string& name, assetDb::AssetType::Enum type)
{
	if (projectNode()) {
		return projectNode()->addFile(name, type);
	}
	return false;
}


bool FolderNode::removeFile(const core::string& name, assetDb::AssetType::Enum type)
{
	if (projectNode()) {
		return projectNode()->removeFile(name, type);
	}
	return false;
}


// ------------------------------------------------------------------


VirtualFolderNode::VirtualFolderNode(const QString& name, int32_t priority)
    : FolderNode(name, NodeType::VirtualFolderNodeType),
      priority_(priority)
{
}

int32_t VirtualFolderNode::priority(void) const
{
    return priority_;
}


// ------------------------------------------------------------------


AssetTypeVirtualFolderNode::AssetTypeVirtualFolderNode(const QString& name, int32_t priority, AssetType::Enum assType) :
	VirtualFolderNode(name, priority),
	assetType_(assType)
{
}

AssetTypeVirtualFolderNode::AssetType::Enum AssetTypeVirtualFolderNode::assetType(void) const
{
	return assetType_;
}


// ------------------------------------------------------------------


ProjectNode::ProjectNode(const QString& projectName)
        : FolderNode(projectName, NodeType::ProjectNodeType)
{
    // project node "manages" itself
    setProjectNode(this);
    setDisplayName(projectName);

    setObjectName("projectnode");
}


QList<ProjectNode*> ProjectNode::subProjectNodes(void) const
{
    return subProjectNodes_;
}


QList<NodesWatcher*> ProjectNode::watchers(void) const
{
    return watchers_;
}


void ProjectNode::registerWatcher(NodesWatcher* pWatcher)
{
    if (!pWatcher) {
        return;
    }

    connect(pWatcher, SIGNAL(destroyed(QObject*)),
            this, SLOT(watcherDestroyed(QObject*)));

    watchers_.append(pWatcher);

    foreach (ProjectNode* pSubProject, subProjectNodes_) {
		pSubProject->registerWatcher(pWatcher);
    }
}

void ProjectNode::unregisterWatcher(NodesWatcher* pWatcher)
{
    if (!pWatcher) {
        return;
    }

    watchers_.removeOne(pWatcher);

    foreach (ProjectNode* pSubProject, subProjectNodes_) {
		pSubProject->unregisterWatcher(pWatcher);
    }
}


void ProjectNode::addProjectNodes(const QList<ProjectNode*>& subProjects)
{
    if (!subProjects.isEmpty())
    {
        QList<FolderNode*> folderNodes;
        foreach (ProjectNode *projectNode, subProjects) {
            folderNodes << projectNode;
        }

        foreach (NodesWatcher* pWatcher, watchers_) {
            emit pWatcher->foldersAboutToBeAdded(this, folderNodes);
        }

        foreach (ProjectNode *project, subProjects)
        {
            BUG_ASSERT(!project->parentFolderNode() || project->parentFolderNode() == this,
                       qDebug("Project node has already a parent"));

            project->setParentFolderNode(this);

            foreach (NodesWatcher* pWatcher, watchers_) {
                project->registerWatcher(pWatcher);
            }

            subFolderNodes_.append(project);
            subProjectNodes_.append(project);
        }

        qSort(subFolderNodes_.begin(), subFolderNodes_.end());
        qSort(subProjectNodes_.begin(), subProjectNodes_.end());

        foreach (NodesWatcher* pWatcher, watchers_) {
            emit pWatcher->foldersAdded();
        }
    }
}


void ProjectNode::removeProjectNodes(const QList<ProjectNode*>& subProjects)
{
    if (!subProjects.isEmpty())
    {
        QList<FolderNode*> toRemove;
        foreach (ProjectNode *projectNode, subProjects) {
            toRemove << projectNode;
        }

        qSort(toRemove.begin(), toRemove.end());

        foreach (NodesWatcher* pWatcher, watchers_) {
            emit pWatcher->foldersAboutToBeRemoved(this, toRemove);
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

        foreach (NodesWatcher* pWatcher, watchers_) {
            emit pWatcher->foldersRemoved();
        }
    }
}

void ProjectNode::watcherDestroyed(QObject* pWatcher)
{
    // cannot use qobject_cast here
    unregisterWatcher(static_cast<NodesWatcher*>(pWatcher));
}

bool ProjectNode::isEnabled(void) const
{
	return true;
}


// ----------------------------------------------------------------------

SessionNode::SessionNode(QObject* pParentObject)
    : FolderNode(QLatin1String("session"), NodeType::SessionNodeType)
{
    setParent(pParentObject);
}

QList<ProjectAction> SessionNode::supportedActions(Node* pNode) const
{
	X_UNUSED(pNode);
    return QList<ProjectAction>();
}

QList<NodesWatcher*> SessionNode::watchers(void) const
{
    return watchers_;
}

void SessionNode::registerWatcher(NodesWatcher* pWatcher)
{
    if (!pWatcher) {
        return;
    }

    connect(pWatcher, SIGNAL(destroyed(QObject*)),
            this, SLOT(watcherDestroyed(QObject*)));

    watchers_.append(pWatcher);

    foreach (ProjectNode *project, projectNodes_) {
        project->registerWatcher(pWatcher);
    }
}


void SessionNode::unregisterWatcher(NodesWatcher* pWatcher)
{
    if (!pWatcher) {
        return;
    }

    watchers_.removeOne(pWatcher);

    foreach (ProjectNode *project, projectNodes_) {
        project->unregisterWatcher(pWatcher);
    }
}

bool SessionNode::isEnabled(void) const
{
	return true;
}



QList<ProjectNode*> SessionNode::projectNodes(void) const
{
    return projectNodes_;
}

void SessionNode::addProjectNodes(const QList<ProjectNode*>& projectNodes)
{
    if (!projectNodes.isEmpty())
    {
        QList<FolderNode*> folderNodes;
        foreach (ProjectNode *projectNode, projectNodes) {
            folderNodes << projectNode;
        }

        foreach (NodesWatcher* pWatcher, watchers_) {
            emit pWatcher->foldersAboutToBeAdded(this, folderNodes);
        }

        foreach (ProjectNode *project, projectNodes)
        {
            BUG_ASSERT(!project->parentFolderNode(),
                qDebug("Project node has already a parent folder"));

            project->setParentFolderNode(this);

            foreach (NodesWatcher* pWatcher, watchers_) {
                project->registerWatcher(pWatcher);
            }
            subFolderNodes_.append(project);
            projectNodes_.append(project);
        }

        qSort(subFolderNodes_);
        qSort(projectNodes_);

        foreach (NodesWatcher* pWatcher, watchers_) {
            emit pWatcher->foldersAdded();
        }
   }
}

void SessionNode::removeProjectNodes(const QList<ProjectNode*>& projectNodes)
{
    if (!projectNodes.isEmpty())
    {
        QList<FolderNode*> toRemove;
        foreach (ProjectNode* pProjectNode, projectNodes) {
            toRemove << pProjectNode;
        }

        qSort(toRemove);

        foreach (NodesWatcher* pWatcher, watchers_) {
            emit pWatcher->foldersAboutToBeRemoved(this, toRemove);
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

        foreach (NodesWatcher* pWatcher, watchers_) {
            emit pWatcher->foldersRemoved();
        }
    }
}

void SessionNode::watcherDestroyed(QObject* pWatcher)
{
    // cannot use qobject_cast here
    unregisterWatcher(static_cast<NodesWatcher*>(pWatcher));
}


NodesWatcher::NodesWatcher(QObject* pParent)
        : QObject(pParent)
{
}


} // namespace AssetExplorer

X_NAMESPACE_END
