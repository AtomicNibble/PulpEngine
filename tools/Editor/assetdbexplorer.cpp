#include "assetdbexplorer.h"
#include "session.h"

#include "project.h"
#include "modproject.h"
#include "assetdbnodes.h"
#include "ParameterAction.h"
#include "ActionManager.h"
#include "ActionContainer.h"
#include "Command.h"

#include "Constants.h"
#include "assetdbconstants.h"

#include "EditorManager.h"

#include "AddAssetDialog.h"
#include "DeleteAssetDialog.h"
#include "RenameAssetDialog.h"

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(editor)

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
	connect(pSessionManager, SIGNAL(startupProjectChanged(Project*)),
		this, SLOT(startupProjectChanged()));

    Context globalcontext(editor::Constants::C_GLOBAL);
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

	reBuildAction_ = new QAction(QIcon(":/misc/img/build.png"), tr("Re-Build"), this);
	reBuildAction_->setStatusTip(tr("ReBuild selected"));

	cleanModAction_ = new QAction(QIcon(":/misc/img/clean.png"), tr("Clean"), this);
	cleanModAction_->setStatusTip(tr("Clean selected"));

	setStartupModAction_ = new ParameterAction(tr("Set as Active Mod"),
		tr("Set \"%1\" as Active Mod"),
		ParameterAction::EnablingMode::AlwaysEnabled, this);

	connect(addNewAssetAction_, SIGNAL(triggered()), this, SLOT(addNewAsset()));
	connect(addNewAssetTypeAction_, SIGNAL(triggered()), this, SLOT(addNewAssetType()));
	connect(buildAction_, SIGNAL(triggered()), this, SLOT(build()));
	connect(reBuildAction_, SIGNAL(triggered()), this, SLOT(buildForce()));
	connect(cleanModAction_, SIGNAL(triggered()), this, SLOT(cleanMod()));
	connect(setStartupModAction_, SIGNAL(triggered()), this, SLOT(setStartupProject()));

    ICommand* pCmd = nullptr;

    // context menus
	ActionContainer* msessionContextMenu = ActionManager::createMenu(Constants::M_SESSIONCONTEXT);
	ActionContainer* mprojectContextMenu = ActionManager::createMenu(Constants::M_PROJECTCONTEXT);
    ActionContainer* mfolderContextMenu = ActionManager::createMenu(Constants::M_FOLDERCONTEXT);
    ActionContainer* mfileContextMenu = ActionManager::createMenu(Constants::M_FILECONTEXT);


	sessionMenu_ = msessionContextMenu->menu();
	projectMenu_ = mprojectContextMenu->menu();
	folderMenu_ = mfolderContextMenu->menu();
	fileMenu_ = mfileContextMenu->menu();

    // Groups
	msessionContextMenu->appendGroup(Constants::G_SESSION_FILES);
	msessionContextMenu->appendGroup(Constants::G_SESSION_BUILD);
	msessionContextMenu->appendGroup(Constants::G_SESSION_REBUILD);
	msessionContextMenu->appendGroup(Constants::G_PROJECT_TREE);
	msessionContextMenu->addSeparator(globalcontext, Constants::G_SESSION_BUILD);

    mprojectContextMenu->appendGroup(Constants::G_PROJECT_FIRST);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_FILES);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_LAST);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_TREE);
	mprojectContextMenu->addSeparator(globalcontext, Constants::G_PROJECT_FILES);

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
	mfileContextMenu->addSeparator(globalcontext, Constants::G_FILE_OPEN);
	mfileContextMenu->addSeparator(globalcontext, Constants::G_FILE_NEW);
	mfileContextMenu->addSeparator(globalcontext, Constants::G_FILE_COMPILE);
	mfileContextMenu->addSeparator(globalcontext, Constants::G_FILE_OTHER);

    // Collapse All.
    {
        const Id treeGroup = Constants::G_PROJECT_TREE;

		pCmd = ActionManager::registerAction(projectTreeCollapseAllAction_,
                               Constants::PROJECTTREE_COLLAPSE_ALL, projecTreeContext);

		msessionContextMenu->addSeparator(globalcontext, treeGroup);
		msessionContextMenu->addAction(pCmd, treeGroup);
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

		msessionContextMenu->addAction(pCmd, treeGroup);
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

	pCmd = ActionManager::registerAction(openAssetAction_, editor::AssetExplorer::Constants::OPEN_ASSET, projecTreeContext);
	pCmd->setDefaultKeySequence(QKeySequence::Open);
	mfileContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FILE_OPEN);


	pCmd = ActionManager::registerAction(renameAssetAction_, editor::AssetExplorer::Constants::RENAME_ASSET, projecTreeContext);
	mfileContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FILE_OTHER);

	pCmd = ActionManager::registerAction(deleteAssetAction_, editor::AssetExplorer::Constants::DELETE_ASSET, projecTreeContext);
	pCmd->setDefaultKeySequence(QKeySequence::Delete);
	mfileContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FILE_OTHER);

	pCmd = ActionManager::registerAction(cutAssetAction_, editor::AssetExplorer::Constants::CUT_ASSET, projecTreeContext);
	pCmd->setDefaultKeySequence(QKeySequence::Cut);
	mfileContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FILE_OTHER);

	pCmd = ActionManager::registerAction(copyAssetAction_, editor::AssetExplorer::Constants::COPY_ASSET, projecTreeContext);
	pCmd->setDefaultKeySequence(QKeySequence::Copy);
	mfileContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FILE_OTHER);

	pCmd = ActionManager::registerAction(pasteAssetAction_, editor::AssetExplorer::Constants::PASTE_ASSET, projecTreeContext);
	pCmd->setDefaultKeySequence(QKeySequence::Paste);
	mfileContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FILE_OTHER);

	pCmd = ActionManager::registerAction(copyAssetNameAction_, editor::AssetExplorer::Constants::COPY_ASSET_NAME, projecTreeContext);
	mfileContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FILE_OTHER);


	// add new
	pCmd = ActionManager::registerAction(addNewAssetAction_, editor::AssetExplorer::Constants::NEW_ASSET, projecTreeContext);
	msessionContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_SESSION_FILES);
	mprojectContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_PROJECT_FIRST);
	mfolderContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FOLDER_FILES);
	mfileContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FILE_NEW);

	// add new Type
	pCmd = ActionManager::registerAction(addNewAssetTypeAction_, editor::AssetExplorer::Constants::NEW_ASSET_TYPE, projecTreeContext);
	pCmd->setAttribute(Command::CommandAttribute::UpdateText);
	mfolderContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FOLDER_FILES);
	mfileContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FILE_NEW);

	// build action
	pCmd = ActionManager::registerAction(buildAction_, editor::AssetExplorer::Constants::BUILD, projecTreeContext);
	msessionContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_SESSION_BUILD);
	mprojectContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_PROJECT_FILES);
	mfolderContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FOLDER_COMPILE);
	mfileContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FILE_COMPILE);

	// re-build action
	pCmd = ActionManager::registerAction(reBuildAction_, editor::AssetExplorer::Constants::REBUILD, projecTreeContext);
	msessionContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_SESSION_REBUILD);
	mprojectContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_PROJECT_FILES);
	mfolderContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FOLDER_COMPILE);
	mfileContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_FILE_COMPILE);

	// clean action
	pCmd = ActionManager::registerAction(cleanModAction_, editor::AssetExplorer::Constants::CLEAN, projecTreeContext);
	mprojectContextMenu->addAction(pCmd, editor::AssetExplorer::Constants::G_PROJECT_FILES);

	// Set start up project
	pCmd = ActionManager::registerAction(setStartupModAction_, Constants::SETSTARTUP, projecTreeContext);
	pCmd->setAttribute(Command::CommandAttribute::UpdateText);
	pCmd->setDescription(setStartupModAction_->text());
	mprojectContextMenu->addAction(pCmd, Constants::G_PROJECT_FIRST);


	connect(ICore::instance(), SIGNAL(saveSettingsRequested()),
		this, SLOT(savePersistentSettings()));

    updateActions();

    return true;
}


bool AssetExplorer::loadMods(void)
{
	assetDb::AssetDB::ModDelegate func;
	func.Bind<AssetExplorer, &AssetExplorer::addMod>(this);
	if (!db_.IterateMods(func)) {
		X_ERROR("AssetExplor", "Failed to iterate mods");
		return false;
	}

	return true;
}

bool AssetExplorer::delayedInit(void)
{

	QTimer::singleShot(10, this, &AssetExplorer::restoreSession);
	return true;
}


bool AssetExplorer::restoreSession(void)
{
	return SessionManager::loadSession();
}



bool AssetExplorer::addMod(assetDb::AssetDB::ModId modId, const core::string& name, const core::Path<char>& outDir)
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
		emit aboutToShowContextMenu(nullptr, pNode);
		pContextMenu = sessionMenu_;
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
				QString newText = tr("New '%1'...").arg(Utils::capitalize(typeStr));

				addNewAssetTypeAction_->setText(newText);
				addNewAssetTypeAction_->setEnabled(enableAddNew);
			}
			else
			{
				addNewAssetTypeAction_->setEnabled(false);

				// need parent type.
				FolderNode* pParent;
				while ((pParent = pFolderNode->parentFolderNode()) != nullptr)
				{
					if (AssetTypeVirtualFolderNode* pParType = qobject_cast<AssetTypeVirtualFolderNode*>(pParent))
					{
						const char* pType = assetDb::AssetType::ToString(pParType->assetType());
						QString typeStr(pType);
						QString newText = tr("New '%1'...").arg(Utils::capitalize(typeStr));

						addNewAssetTypeAction_->setText(newText);
						addNewAssetTypeAction_->setEnabled(enableAddNew);
						break;
					}

					pFolderNode = pParent;
				}
			}

			addNewAssetAction_->setEnabled(enableAddNew);
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
			QString newText = tr("New '%1'...").arg(Utils::capitalize(typeStr));

			addNewAssetTypeAction_->setText(newText);

			addNewAssetAction_->setEnabled(enableAddNew);
			addNewAssetTypeAction_->setEnabled(enableAddNew);
			deleteAssetAction_->setEnabled(enableDelete);
			renameAssetAction_->setEnabled(enableRename);
		}
	}
	else
	{
		addNewAssetAction_->setEnabled(true);
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

void AssetExplorer::startupProjectChanged(void)
{

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
	BUG_ASSERT(currentNode_, return);

	if (FileNode* pFileNode = qobject_cast<FileNode*>(currentNode_)) {
		EditorManager::openEditor(pFileNode->name(), pFileNode->assetType(), editor::Constants::ASSETPROP_EDITOR_ID);
	}
}

void AssetExplorer::renameAsset(void)
{
	BUG_ASSERT(currentNode_, return);

	if (FileNode* pFileNode = qobject_cast<FileNode*>(currentNode_)) {
		RenameAssetDialog dlg(ICore::mainWindow(), db_, pFileNode->assetType(), pFileNode->name());

		dlg.exec();
	}
}

void AssetExplorer::deleteAsset(void)
{
	BUG_ASSERT(currentNode_, return);

	if (FileNode* pFileNode = qobject_cast<FileNode*>(currentNode_)) {
		DeleteAssetDialog dlg(ICore::mainWindow(), db_, pFileNode->assetType(), pFileNode->name());

		if (!dlg.loadInfo()) {
			return;
		}

		if (dlg.exec() == QDialog::Accepted)
		{
		//	FolderNode* pFolder = pFileNode->parentFolderNode();

		//	pFolder->removeFile(dlg.getName(), dlg.getType());
		}
	}
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
	AddAssetDialog dialog(ICore::mainWindow(), db_);
	if (currentProject_) {
		dialog.setPrefredMod(currentProject_->displayName());
	}
	else if (Project* pProject = SessionManager::startupProject()) {
		dialog.setPrefredMod(pProject->displayName());
	}

	auto res = dialog.exec();
	if (res == QDialog::Accepted)
	{
		// update the tree.
		// we need the types folder.
		// we i guess we should really get the mod it was placed in then
		// delegate adding to that.

		//pAssetTypeFolder->addFile(dialog.getName(), dialog.getType());
	}
}

void AssetExplorer::addNewAssetType(void)
{
	BUG_ASSERT(currentNode_, return);

	if (currentNode_->nodeType() != NodeType::FileNodeType &&
		currentNode_->nodeType() != NodeType::VirtualFolderNodeType &&
		currentNode_->nodeType() != NodeType::FolderNodeType) {
		return;
	}

	AddAssetDialog dialog(ICore::mainWindow(), db_);
	AssetTypeVirtualFolderNode* pAssetTypeFolder = nullptr;
	

	auto findVF = [&](FolderNode* pCurFolder) {

		dialog.setNameHint(pCurFolder->name() + "/");

		FolderNode* pParent;
		while ((pParent = pCurFolder->parentFolderNode()) != nullptr)
		{
			if (pParent->nodeType() == NodeType::VirtualFolderNodeType)
			{
				if (pAssetTypeFolder = qobject_cast<AssetTypeVirtualFolderNode*>(pParent))
				{
					break;
				}
			}

			pCurFolder = pParent;
		}
	};
	
	if (currentNode_->nodeType() == NodeType::VirtualFolderNodeType)
	{
		pAssetTypeFolder = qobject_cast<AssetTypeVirtualFolderNode*>(currentNode_);
	}
	else if (currentNode_->nodeType() == NodeType::FolderNodeType)
	{
		FolderNode* pCurFolder = qobject_cast<FolderNode*>(currentNode_);
		findVF(pCurFolder);
	}
	else if (currentNode_->nodeType() == NodeType::FileNodeType)
	{
		FileNode* pFileNode = qobject_cast<FileNode*>(currentNode_);
		if (pFileNode)
		{
			FolderNode* pFolder = pFileNode->parentFolderNode();
			if (pFolder)
			{
				findVF(pFolder);
			}
		}
	}

	if (pAssetTypeFolder) {
		dialog.setAssetType(pAssetTypeFolder->assetType());
	}
	if (currentProject_) {
		dialog.setPrefredMod(currentProject_->displayName());
	}

	if (dialog.exec() == QDialog::Accepted) 
	{
		pAssetTypeFolder->addFile(dialog.getName(), dialog.getType());
	}
}

void AssetExplorer::build(void)
{
	BUG_ASSERT(currentNode_, return);

	if (currentNode_->nodeType() != NodeType::FileNodeType &&
		currentNode_->nodeType() != NodeType::FolderNodeType &&
		currentNode_->nodeType() != NodeType::VirtualFolderNodeType &&
		currentNode_->nodeType() != NodeType::ProjectNodeType) {
		return;
	}


//	currentNode_->build(conHost_);
}


void AssetExplorer::buildForce(void)
{
	BUG_ASSERT(currentNode_, return);

	if (currentNode_->nodeType() != NodeType::FileNodeType &&
		currentNode_->nodeType() != NodeType::FolderNodeType &&
		currentNode_->nodeType() != NodeType::VirtualFolderNodeType &&
		currentNode_->nodeType() != NodeType::ProjectNodeType) {
		return;
	}


//	currentNode_->build(conHost_, true);
}


void AssetExplorer::cleanMod(void)
{
	BUG_ASSERT(currentNode_, return);

	if (currentNode_->nodeType() != NodeType::ProjectNodeType) {
		return;
	}

	if (ProjectNode* pProjectNode = qobject_cast<ProjectNode*>(currentNode_)) {
	//	pProjectNode->clean(conHost_);
	}
}


void AssetExplorer::updateActions(void)
{
	qDebug() << "AssetExplorer::updateActions";

	Project* pProject = SessionManager::startupProject();
	QString projectName = pProject ? pProject->displayName() : QString();
	QString projectNameContextMenu = currentProject_ ? currentProject_->displayName() : QString();


	setStartupModAction_->setParameter(projectNameContextMenu);

	qDebug() << "Project name: " << projectName;
	qDebug() << "Contex name: " << projectNameContextMenu;
}


void AssetExplorer::savePersistentSettings(void)
{

	SessionManager::save();

}


} // namespace AssetExplorer

X_NAMESPACE_END
