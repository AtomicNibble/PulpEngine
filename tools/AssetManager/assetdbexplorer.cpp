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


	// asset actions
	openAssetAction_ = new QAction(tr("Open"), this);
	openAssetAction_->setStatusTip(tr("Open the asset in editor"));
	renameAssetAction_ = new QAction(tr("Rename..."), this);
	renameAssetAction_->setStatusTip(tr("Rename selected asset"));
	deleteAssetAction_ = new QAction(tr("Delete"), this);
	deleteAssetAction_->setStatusTip(tr("Delete the selected asset"));
	cutAssetAction_ = new QAction(QIcon(":/misc/img/Cut.png"), tr("Cut"), this);
	copyAssetAction_ = new QAction(QIcon(":/misc/img/Copy.png"), tr("Copy"), this);
	copyAssetNameAction_ = new QAction(QIcon(":/misc/img/Copy.png"), tr("Copy Asset Name"), this);
	pasteAssetAction_ = new QAction(QIcon(":/misc/img/Paste.png"), tr("Paste"), this);


	connect(openAssetAction_, SIGNAL(triggered()), this, SLOT(openAsset()));
	connect(renameAssetAction_, SIGNAL(triggered()), this, SLOT(renameAsset()));
	connect(deleteAssetAction_, SIGNAL(triggered()), this, SLOT(deleteAsset()));
	connect(cutAssetAction_, SIGNAL(triggered()), this, SLOT(cutAsset()));
	connect(copyAssetAction_, SIGNAL(triggered()), this, SLOT(copyAsset()));
	connect(pasteAssetAction_, SIGNAL(triggered()), this, SLOT(pasteAsset()));
	connect(copyAssetNameAction_, SIGNAL(triggered()), this, SLOT(copyAssetName()));

	addNewAssetAction_ = new QAction(QIcon(":/misc/img/NewFile.png"), tr("New Asset..."), this);
	addNewAssetAction_->setStatusTip(tr("Create new asset"));

	addNewAssetTypeAction_ = new QAction(QIcon(":/misc/img/NewFile.png"), tr("New ?..."), this);
	addNewAssetTypeAction_->setStatusTip(tr("Create new asset"));

	projectTreeCollapseAllAction_ = new QAction(tr("Collapse All"), this);
	projectTreeExpandAllAction_ = new QAction(tr("Expand All"), this);
	projectTreeExpandBelowAction_ = new QAction(tr("Expand Below"), this);

	buildAction_ = new QAction(QIcon(":/misc/img/build.png"), tr("Build"), this);
	buildAction_->setStatusTip(tr("Build selected"));

	connect(addNewAssetAction_, SIGNAL(triggered()), this, SLOT(addNewAsset()));
	connect(addNewAssetTypeAction_, SIGNAL(triggered()), this, SLOT(addNewAssetType()));
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
    mfolderContextMenu->appendGroup(Constants::G_FOLDER_COMPILE);
    mfolderContextMenu->appendGroup(Constants::G_FOLDER_CONFIG);
    mfolderContextMenu->appendGroup(Constants::G_PROJECT_TREE);
	mfolderContextMenu->addSeparator(globalcontext, Constants::G_FOLDER_COMPILE);


    mfileContextMenu->appendGroup(Constants::G_FILE_OPEN);
	mfileContextMenu->appendGroup(Constants::G_FILE_NEW);
	mfileContextMenu->appendGroup(Constants::G_FILE_COMPILE);
	mfileContextMenu->appendGroup(Constants::G_FILE_OTHER);
    mfileContextMenu->appendGroup(Constants::G_PROJECT_TREE);

    // Separators
    mprojectContextMenu->addSeparator(globalcontext, Constants::G_PROJECT_FILES);
	mfileContextMenu->addSeparator(globalcontext, Constants::G_FILE_OPEN);
	mfileContextMenu->addSeparator(globalcontext, Constants::G_FILE_NEW);
	mfileContextMenu->addSeparator(globalcontext, Constants::G_FILE_COMPILE);
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

	// Expand All.
	{
		const Id treeGroup = Constants::G_PROJECT_TREE;

		pCmd = ActionManager::registerAction(projectTreeExpandAllAction_,
			Constants::PROJECTTREE_EXPAND_ALL, projecTreeContext);

		mprojectContextMenu->addAction(pCmd, treeGroup);
		mfolderContextMenu->addAction(pCmd, treeGroup);
		mfileContextMenu->addAction(pCmd, treeGroup);
	}

	// Expand Below.
	{
		const Id treeGroup = Constants::G_PROJECT_TREE;

		pCmd = ActionManager::registerAction(projectTreeExpandBelowAction_,
			Constants::PROJECTTREE_EXPAND_BELOW, projecTreeContext);

		mprojectContextMenu->addAction(pCmd, treeGroup);
		mfolderContextMenu->addAction(pCmd, treeGroup);
		mfileContextMenu->addAction(pCmd, treeGroup);
	}

	// Shieeeeeeeeet for the file.

	pCmd = ActionManager::registerAction(openAssetAction_, assman::AssetExplorer::Constants::OPEN_ASSET, projecTreeContext);
	pCmd->setDefaultKeySequence(QKeySequence::Open);
	mfileContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_FILE_OPEN);


	pCmd = ActionManager::registerAction(renameAssetAction_, assman::AssetExplorer::Constants::RENAME_ASSET, projecTreeContext);
	mfileContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_FILE_OTHER);

	pCmd = ActionManager::registerAction(deleteAssetAction_, assman::AssetExplorer::Constants::DELETE_ASSET, projecTreeContext);
	pCmd->setDefaultKeySequence(QKeySequence::Delete);
	mfileContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_FILE_OTHER);

	pCmd = ActionManager::registerAction(cutAssetAction_, assman::AssetExplorer::Constants::CUT_ASSET, projecTreeContext);
	pCmd->setDefaultKeySequence(QKeySequence::Cut);
	mfileContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_FILE_OTHER);

	pCmd = ActionManager::registerAction(copyAssetAction_, assman::AssetExplorer::Constants::COPY_ASSET, projecTreeContext);
	pCmd->setDefaultKeySequence(QKeySequence::Copy);
	mfileContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_FILE_OTHER);

	pCmd = ActionManager::registerAction(pasteAssetAction_, assman::AssetExplorer::Constants::PASTE_ASSET, projecTreeContext);
	pCmd->setDefaultKeySequence(QKeySequence::Paste);
	mfileContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_FILE_OTHER);

	pCmd = ActionManager::registerAction(copyAssetNameAction_, assman::AssetExplorer::Constants::COPY_ASSET_NAME, projecTreeContext);
	mfileContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_FILE_OTHER);


	// add new
	pCmd = ActionManager::registerAction(addNewAssetAction_, assman::AssetExplorer::Constants::NEW_ASSET, projecTreeContext);
	mprojectContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_PROJECT_FIRST);
	mfolderContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_FOLDER_FILES);
	mfileContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_FILE_NEW);

	// add new Type
	pCmd = ActionManager::registerAction(addNewAssetTypeAction_, assman::AssetExplorer::Constants::NEW_ASSET_TYPE, projecTreeContext);
	pCmd->setAttribute(Command::CommandAttribute::UpdateText);
	mfolderContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_FOLDER_FILES);
	mfileContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_FILE_NEW);

	// build action
	pCmd = ActionManager::registerAction(buildAction_, assman::AssetExplorer::Constants::BUILD, projecTreeContext);
	mprojectContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_PROJECT_FILES);
	mfolderContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_FOLDER_COMPILE);
	mfileContextMenu->addAction(pCmd, assman::AssetExplorer::Constants::G_FILE_COMPILE);



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
	projectTreeExpandAllAction_->disconnect(SIGNAL(triggered()));
	projectTreeExpandBelowAction_->disconnect(SIGNAL(triggered()));
	connect(projectTreeCollapseAllAction_, SIGNAL(triggered()), pView, SLOT(collapseAll()));
	connect(projectTreeExpandAllAction_, SIGNAL(triggered()), pView, SLOT(expandAll()));
	connect(projectTreeExpandBelowAction_, SIGNAL(triggered()), pView, SLOT(expandBelow()));

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

	if (currentNode_ && currentNode_->projectNode())
	{
		auto actions = currentNode_->supportedActions(currentNode_);

		const bool enableAddNew = actions.contains(ProjectAction::AddNewFile);
		const bool enableDelete = actions.contains(ProjectAction::EraseFile);
		const bool enableRename = actions.contains(ProjectAction::Rename);

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
		if (FolderNode* pFolderNode = qobject_cast<FolderNode*>(currentNode_))
		{
			// i want to know asset type.
			// and update the name.

			// Also handles 
			if(AssetTypeVirtualFolderNode* pAssetFolder = qobject_cast<AssetTypeVirtualFolderNode*>(currentNode_))
			{
				const char* pType = assetDb::AssetType::ToString(pAssetFolder->assetType());
				QString typeStr(pType);
				QString newText = tr("New '%1'...").arg(capitalize(typeStr));

				addNewAssetTypeAction_->setText(newText);
				addNewAssetTypeAction_->setEnabled(enableAddNew);
			}
			else
			{
				addNewAssetTypeAction_->setEnabled(false);
			}

			addNewAssetAction_->setEnabled(enableAddNew);
			addNewAssetTypeAction_->setEnabled(enableAddNew);
			renameAssetAction_->setEnabled(enableRename);
		}
		else if (FileNode* pFileNode = qobject_cast<FileNode*>(currentNode_))
		{
			// Enable and show remove / delete in magic ways:
			// If both are disabled show Remove
			// If both are enabled show both (can't happen atm)
			// If only removeFile is enabled only show it
			// If only deleteFile is enable only show it

			const char* pType = assetDb::AssetType::ToString(pFileNode->assetType());
			QString typeStr(pType);
			QString newText = tr("New '%1'...").arg(capitalize(typeStr));

			addNewAssetTypeAction_->setText(newText);

			addNewAssetAction_->setEnabled(enableAddNew);
			addNewAssetTypeAction_->setEnabled(enableAddNew);
			deleteAssetAction_->setEnabled(enableDelete);
			renameAssetAction_->setEnabled(enableRename);
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

void AssetExplorer::openAsset(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}

void AssetExplorer::renameAsset(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}

void AssetExplorer::deleteAsset(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}

void AssetExplorer::cutAsset(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}

void AssetExplorer::copyAsset(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}

void AssetExplorer::pasteAsset(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}

void AssetExplorer::copyAssetName(void)
{
	BUG_ASSERT(currentNode_ && currentNode_->nodeType() == NodeType::FileNodeType, return);

	FileNode* pFileNode = qobject_cast<FileNode*>(currentNode_);

	QClipboard* pClipboard = QApplication::clipboard();
	pClipboard->setText(pFileNode->name());
}

void AssetExplorer::addNewAsset(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}

void AssetExplorer::addNewAssetType(void)
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
