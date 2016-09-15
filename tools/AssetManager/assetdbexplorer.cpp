#include "assetdbexplorer.h"
#include "session.h"

#include "project.h"
#include "modproject.h"
#include "assetdbnodes.h"
#include "ActionManager.h"
#include "ActionContainer.h"
#include "Command.h"

#include "Constants.h"
#include "assetdbconstants.h"

#include <../AssetDB/AssetDB.h>


X_NAMESPACE_BEGIN(assman)

namespace AssetExplorer
{


AssetExplorer *AssetExplorer::instance_ = nullptr;
AssetExplorer *AssetExplorer::instance()
{
    return instance_;
}


AssetExplorer::AssetExplorer(assetDb::AssetDB& db)  :
	db_(db),
    currentProject_(nullptr),
    currentNode_(nullptr)
{
    instance_ = this;
}

AssetExplorer::~AssetExplorer()
{
}


bool AssetExplorer::init(void)
{

    SessionManager* pSessionManager = new SessionManager(this);

    connect(pSessionManager, SIGNAL(projectAdded(Project*)),
            this, SIGNAL(fileListChanged()));
    connect(pSessionManager, SIGNAL(projectRemoved(Project*)),
            this, SIGNAL(fileListChanged()));
    connect(pSessionManager, SIGNAL(projectAdded(Project*)),
            this, SLOT(projectAdded(Project*)));
    connect(pSessionManager, SIGNAL(projectRemoved(Project*)),
            this, SLOT(projectRemoved(Project*)));
    connect(pSessionManager, SIGNAL(projectDisplayNameChanged(Project*)),
            this, SLOT(projectDisplayNameChanged(Project*)));


    Context globalcontext(assman::Constants::C_GLOBAL);
    Context projecTreeContext(Constants::C_ASSETDB_EXPLORER);


	addNewFileAction_ = new QAction(tr("Add New..."), this);
	deleteFileAction_ = new QAction(tr("Delete File..."), this);
	renameFileAction_ = new QAction(tr("Rename..."), this);
	openFileAction_ = new QAction(tr("Open File"), this);
	projectTreeCollapseAllAction_ = new QAction(tr("Collapse All"), this);
	buildAction_ = new QAction(tr("Build"), this);


    connect(addNewFileAction_, SIGNAL(triggered()), this, SLOT(addNewFile()));
    connect(deleteFileAction_, SIGNAL(triggered()), this, SLOT(deleteFile()));
    connect(renameFileAction_, SIGNAL(triggered()), this, SLOT(renameFile()));
	connect(openFileAction_, SIGNAL(triggered()), this, SLOT(openFile()));
	connect(buildAction_, SIGNAL(triggered()), this, SLOT(build()));

    ICommand* pCmd = nullptr;

    // context menus
    ActionContainer* mprojectContextMenu = ActionManager::createMenu(Constants::M_PROJECTCONTEXT);
    ActionContainer* mfolderContextMenu = ActionManager::createMenu(Constants::M_FOLDERCONTEXT);
    ActionContainer* mfileContextMenu = ActionManager::createMenu(Constants::M_FILECONTEXT);


	projectMenu_ = mprojectContextMenu->menu();
	folderMenu_ = mfolderContextMenu->menu();
	fileMenu_ = mfileContextMenu->menu();

    // Groups
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_FIRST);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_FILES);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_LAST);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_TREE);

    mfolderContextMenu->appendGroup(Constants::G_FOLDER_FILES);
    mfolderContextMenu->appendGroup(Constants::G_FOLDER_OTHER);
    mfolderContextMenu->appendGroup(Constants::G_FOLDER_CONFIG);
    mfolderContextMenu->appendGroup(Constants::G_PROJECT_TREE);

    mfileContextMenu->appendGroup(Constants::G_FILE_OPEN);
    mfileContextMenu->appendGroup(Constants::G_FILE_OTHER);
    mfileContextMenu->appendGroup(Constants::G_PROJECT_TREE);

    // Separators
    mprojectContextMenu->addSeparator(globalcontext, Constants::G_PROJECT_FILES);
    mfileContextMenu->addSeparator(globalcontext, Constants::G_FILE_OTHER);

    // Collapse All.
    {
        const Id treeGroup = Constants::G_PROJECT_TREE;

		pCmd = ActionManager::registerAction(projectTreeCollapseAllAction_,
                               Constants::PROJECTTREE_COLLAPSE_ALL, projecTreeContext);

        mprojectContextMenu->addSeparator(globalcontext, treeGroup);
        mprojectContextMenu->addAction(pCmd, treeGroup);
        mfolderContextMenu->addSeparator(globalcontext, treeGroup);
        mfolderContextMenu->addAction(pCmd, treeGroup);
        mfileContextMenu->addSeparator(globalcontext, treeGroup);
        mfileContextMenu->addAction(pCmd, treeGroup);
    }


    // add new file action
	pCmd = ActionManager::registerAction(addNewFileAction_, assman::AssetExplorer::Constants::ADDNEWFILE, projecTreeContext);
    mprojectContextMenu->addAction(pCmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(pCmd, Constants::G_FOLDER_FILES);

    // delete file action
    pCmd = ActionManager::registerAction(deleteFileAction_, assman::AssetExplorer::Constants::DELETEFILE, projecTreeContext);
    pCmd->setDefaultKeySequence(QKeySequence::Delete);
    mfileContextMenu->addAction(pCmd, Constants::G_FILE_OTHER);

    // renamefile action
    pCmd = ActionManager::registerAction(renameFileAction_, assman::AssetExplorer::Constants::RENAMEFILE, projecTreeContext);
    mfileContextMenu->addAction(pCmd, Constants::G_FILE_OTHER);

	// build action
	pCmd = ActionManager::registerAction(buildAction_, assman::AssetExplorer::Constants::BUILD, projecTreeContext);
	mfileContextMenu->addAction(pCmd, Constants::G_FILE_OTHER);
	mfolderContextMenu->addAction(pCmd, Constants::G_FOLDER_FILES);
	mprojectContextMenu->addAction(pCmd, Constants::G_PROJECT_FILES);


    updateActions();
    return true;
}


bool AssetExplorer::loadMods(void)
{
	core::Delegate<bool(assetDb::AssetDB::ModId id, const core::string& name, core::Path<char>& outDir)> func;
	func.Bind<AssetExplorer, &AssetExplorer::addMod>(this);
	if (!db_.IterateMods(func)) {
		X_ERROR("AssetExplor", "Failed to iterate mods");
		return false;
	}

	return true;
}

bool AssetExplorer::addMod(assetDb::AssetDB::ModId modId, const core::string& name, core::Path<char>& outDir)
{
	X_UNUSED(outDir);

	ModProject* pMod = new ModProject(db_, QString::fromUtf8(name.c_str()), modId);
	pMod->loadAssetTypeNodes();
	SessionManager::addProject(pMod);

	return true;
}

Project* AssetExplorer::currentProject(void)
{
    Project* pProject = instance_->currentProject_;

	if (pProject) {
		qDebug() << "AssetExplorer::currentProject returns " << pProject->displayName();
	}
	else {
		qDebug() << "AssetExplorer::currentProject returns 0";
	}

    return pProject;
}

Node* AssetExplorer::currentNode(void) const
{
    return currentNode_;
}

void AssetExplorer::setCurrentNode(Node* pNode)
{
    setCurrent(SessionManager::projectForNode(pNode), QString(), pNode);
}


void AssetExplorer::setCurrent(Project* pProject, const QString& name_, Node* pNode)
{
    if (debugLogging) {
        qDebug() << "ProjectExplorer - setting path to " << (name_)
                << " and project to " << (pProject ? pProject->displayName() : QLatin1String("0"));
    }

	QString name(name_);

	if (pNode) {
		name = pNode->name();
	}
	else {
	//	pNode = SessionManager::no(filePath, project);
	}

	bool projectChanged = false;
	if (currentProject_ != pProject)
	{
		if (currentProject_) {

		}
		if (pProject) {

		}
		projectChanged = true;
	}

	currentProject_ = pProject;

	if (projectChanged || currentNode_ != pNode) {
		currentNode_ = pNode;
		if (debugLogging) {
			qDebug() << "ProjectExplorer - currentNodeChanged(" << (pNode ? pNode->name() : QLatin1String("0")) << ", " << (pProject ? pProject->displayName() : QLatin1String("0")) << ')';
		}
		emit currentNodeChanged(currentNode_, pProject);
		updateContextMenuActions();
	}
	if (projectChanged) {
		if (debugLogging) {
			qDebug() << "ProjectExplorer - currentProjectChanged(" << (pProject ? pProject->displayName() : QLatin1String("0")) << ')';
		}
		emit currentProjectChanged(pProject);
		updateActions();
	}

}


void AssetExplorer::showContextMenu(QWidget* pView, const QPoint& globalPos, Node* pNode)
{
    qDebug() << "AssetExplorer::showContextMenu";
	X_UNUSED(pView);
	X_UNUSED(globalPos);
	X_UNUSED(pNode);


	QMenu* pContextMenu = nullptr;

	if (!pNode) {
		pNode = SessionManager::sessionNode();
	}

	if (pNode->nodeType() != NodeType::SessionNodeType)
	{
		Project* pProject = SessionManager::projectForNode(pNode);
		setCurrentNode(pNode);

		emit aboutToShowContextMenu(pProject, pNode);
		switch (pNode->nodeType())
		{
		case NodeType::ProjectNodeType:
			//    if (node->parentFolderNode() == SessionManager::sessionNode())
			pContextMenu = projectMenu_;
			//     else
			//         contextMenu = m_subProjectMenu;
			break;
		case NodeType::VirtualFolderNodeType:
		case NodeType::FolderNodeType:
			pContextMenu = folderMenu_;
			break;
		case NodeType::FileNodeType:
			//        populateOpenWithMenu();
			pContextMenu = fileMenu_;
			break;
		default:
			qWarning("ProjectExplorerPlugin::showContextMenu - Missing handler for node type");
		}
	}
	else { // session item
		   //   emit aboutToShowContextMenu(0, node);
		   //   contextMenu = m_sessionContextMenu;
	}

	updateContextMenuActions();

	projectTreeCollapseAllAction_->disconnect(SIGNAL(triggered()));
	connect(projectTreeCollapseAllAction_, SIGNAL(triggered()), pView, SLOT(collapseAll()));

	if (pContextMenu && pContextMenu->actions().count() > 0) {
		pContextMenu->popup(globalPos);
	}
}


void AssetExplorer::renameFile(Node* pNode, const QString& to)
{
	X_UNUSED(pNode);
	X_UNUSED(to);

	X_ASSERT_NOT_IMPLEMENTED();
}


void AssetExplorer::updateContextMenuActions(void)
{
    qDebug() << "AssetExplorer::updateContextMenuActions";

	addNewFileAction_->setEnabled(false);
	deleteFileAction_->setEnabled(false);
	renameFileAction_->setEnabled(false);

	if (currentNode_ && currentNode_->projectNode())
	{
		auto actions = currentNode_->supportedActions(currentNode_);

		if (ProjectNode* pn = qobject_cast<ProjectNode*>(currentNode_))
		{
			if (pn == currentProject_->rootProjectNode())
			{
				buildAction_->setVisible(true);
				buildAction_->setEnabled(true);

			}
			else
			{

			}
		}
		if (qobject_cast<FolderNode*>(currentNode_))
		{
			// Also handles ProjectNode
			addNewFileAction_->setEnabled(actions.contains(ProjectAction::AddNewFile));
			renameFileAction_->setEnabled(actions.contains(ProjectAction::Rename));
		}
		else if (qobject_cast<FileNode*>(currentNode_))
		{
			// Enable and show remove / delete in magic ways:
			// If both are disabled show Remove
			// If both are enabled show both (can't happen atm)
			// If only removeFile is enabled only show it
			// If only deleteFile is enable only show it
			bool enableDelete = actions.contains(ProjectAction::EraseFile);

			deleteFileAction_->setEnabled(enableDelete);
			deleteFileAction_->setVisible(enableDelete);

			renameFileAction_->setEnabled(actions.contains(ProjectAction::Rename));
		}
	}
}



void AssetExplorer::projectAdded(Project* pPro)
{
    connect(pPro, SIGNAL(buildConfigurationEnabledChanged()),
            this, SLOT(updateActions()));

	X_ASSERT_NOT_IMPLEMENTED();
}

void AssetExplorer::projectRemoved(Project* pPro)
{
    disconnect(pPro, SIGNAL(buildConfigurationEnabledChanged()),
               this, SLOT(updateActions()));

	X_ASSERT_NOT_IMPLEMENTED();

}

void AssetExplorer::projectDisplayNameChanged(Project* pPro)
{
	X_UNUSED(pPro);
	X_ASSERT_NOT_IMPLEMENTED();

    updateActions();
}


void AssetExplorer::newProject(void)
{
    qDebug() << "AssetExplorer::newProject";

	X_ASSERT_NOT_IMPLEMENTED();

    updateActions();
}


void AssetExplorer::setStartupProject(void)
{
    setStartupProject(currentProject_);
}

void AssetExplorer::setStartupProject(Project* pProject)
{
    qDebug() << "AssetExplorer::setStartupProject";

    if (!pProject) {
        return;
    }

    SessionManager::setStartupProject(pProject);
    updateActions();
}

void AssetExplorer::addNewFile(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}

void AssetExplorer::openFile(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}

void AssetExplorer::removeFile(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}

void AssetExplorer::deleteFile(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}

void AssetExplorer::renameFile(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}



void AssetExplorer::updateActions(void)
{
	qDebug() << "AssetExplorer::updateActions";

	Project* pProject = SessionManager::startupProject();
	QString projectName = pProject ? pProject->displayName() : QString();
	QString projectNameContextMenu = currentProject_ ? currentProject_->displayName() : QString();


	qDebug() << "Project name: " << projectName;
	qDebug() << "Contex name: " << projectNameContextMenu;
}



} // namespace AssetExplorer

X_NAMESPACE_END
