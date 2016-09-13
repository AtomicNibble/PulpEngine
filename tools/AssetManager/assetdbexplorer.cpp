#include "assetdbexplorer.h"
#include "session.h"

#include "project.h"
#include "assetdbnodes.h"


X_NAMESPACE_BEGIN(assman)

namespace AssetExplorer
{


AssetExplorer *AssetExplorer::instance_ = nullptr;
AssetExplorer *AssetExplorer::instance()
{
    return instance_;
}


AssetExplorer::AssetExplorer()  :
    currentProject_(nullptr),
    currentNode_(nullptr)
{
    instance_ = this;
}

AssetExplorer::~AssetExplorer()
{
}


bool AssetExplorer::init(QString* errorMessage)
{
    Q_UNUSED(errorMessage);


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


#if 0
    Context globalcontext(Bug::Constants::C_GLOBAL);
    Context projecTreeContext(Constants::C_PROJECT_TREE);


    m_addNewFileAction = new QAction(tr("Add New..."), this);
    m_addExistingFilesAction = new QAction(tr("Add Existing Files..."), this);
    m_removeFileAction = new QAction(tr("Remove File..."), this);
    m_deleteFileAction = new QAction(tr("Delete File..."), this);
    m_renameFileAction = new QAction(tr("Rename..."), this);
    m_openFileAction = new QAction(tr("Open File"), this);
    m_openContaingFolderAction = new QAction(tr("Open Containing Folder"), this);
    m_projectTreeCollapseAllAction = new QAction(tr("Collapse All"), this);
    m_unloadAction = new QAction(tr("Unload Project"), this);

    m_setStartupProjectAction = new Utils::ParameterAction(tr("Set as Active Project"),
                                                              tr("Set \"%1\" as Active Project"),
                                                              Utils::ParameterAction::AlwaysEnabled, this);

    connect(m_addNewFileAction, SIGNAL(triggered()), this, SLOT(addNewFile()));
    connect(m_addExistingFilesAction, SIGNAL(triggered()), this, SLOT(addExistingFiles()));
    connect(m_removeFileAction, SIGNAL(triggered()), this, SLOT(removeFile()));
    connect(m_deleteFileAction, SIGNAL(triggered()), this, SLOT(deleteFile()));
    connect(m_renameFileAction, SIGNAL(triggered()), this, SLOT(renameFile()));
    connect(m_openFileAction, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(m_setStartupProjectAction, SIGNAL(triggered()), this, SLOT(setStartupProject()));
    connect(m_unloadAction, SIGNAL(triggered()), this, SLOT(unloadProject()));

    Command* cmd;

    // context menus
    ActionContainer *mprojectContextMenu = ActionManager::createMenu(Constants::M_PROJECTCONTEXT);
    ActionContainer *mfolderContextMenu = ActionManager::createMenu(Constants::M_FOLDERCONTEXT);
    ActionContainer *mfileContextMenu = ActionManager::createMenu(Constants::M_FILECONTEXT);


    m_projectMenu = mprojectContextMenu->menu();
    m_folderMenu = mfolderContextMenu->menu();
    m_fileMenu = mfileContextMenu->menu();

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

        cmd = ActionManager::registerAction(m_projectTreeCollapseAllAction,
                               Constants::PROJECTTREE_COLLAPSE_ALL, projecTreeContext);

        mprojectContextMenu->addSeparator(globalcontext, treeGroup);
        mprojectContextMenu->addAction(cmd, treeGroup);
        mfolderContextMenu->addSeparator(globalcontext, treeGroup);
        mfolderContextMenu->addAction(cmd, treeGroup);
        mfileContextMenu->addSeparator(globalcontext, treeGroup);
        mfileContextMenu->addAction(cmd, treeGroup);
    }


    // add new file action
    cmd = ActionManager::registerAction(m_addNewFileAction, AssetExplorer::Constants::ADDNEWFILE, projecTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);


    // add existing file action
    cmd = ActionManager::registerAction(m_addExistingFilesAction, AssetExplorer::Constants::ADDEXISTINGFILES, projecTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // Open file
    cmd = ActionManager::registerAction(m_openFileAction, AssetExplorer::Constants::OPENFILE, projecTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);

    // Open Containing Folder
    cmd = ActionManager::registerAction(m_openContaingFolderAction, AssetExplorer::Constants::OPENCONTAININGFOLDER, projecTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // remove file action
    cmd = ActionManager::registerAction(m_removeFileAction, AssetExplorer::Constants::REMOVEFILE, projecTreeContext);
    cmd->setDefaultKeySequence(QKeySequence::Delete);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    // delete file action
    cmd = ActionManager::registerAction(m_deleteFileAction, AssetExplorer::Constants::DELETEFILE, projecTreeContext);
    cmd->setDefaultKeySequence(QKeySequence::Delete);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    // renamefile action
    cmd = ActionManager::registerAction(m_renameFileAction, AssetExplorer::Constants::RENAMEFILE, projecTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    // Unload Project
    cmd = ActionManager::registerAction(m_unloadAction, AssetExplorer::Constants::UNLOADPROJECT, globalcontext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);

    // Set start up project
    cmd = ActionManager::registerAction(m_setStartupProjectAction, AssetExplorer::Constants::SETSTARTUP, projecTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(m_setStartupProjectAction->text());
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FIRST);

    QSettings *s = ICore::settings();

    // Load recent projects.
    m_recentProjects = s->value(QLatin1String("recentProjectList")).toStringList();


     // Get the file menu
    ActionContainer *mfile = ActionManager::actionContainer(Bug::Constants::M_FILE);

    // add recent projects menu
    ActionContainer *mrecent = ActionManager::createMenu(Constants::M_RECENTPROJECTS);
    mrecent->menu()->setTitle(tr("Recent P&rojects"));
    mrecent->setOnAllDisabledBehavior(ActionContainer::Show);

    // Add the menu File->recent Project->*
    mfile->addMenu(mrecent, Bug::Constants::G_FILE_RECENT);
    connect(mfile->menu(), SIGNAL(aboutToShow()), this, SLOT(updateRecentProjectMenu()));


    // We have settings that can be saved.
    connect(ICore::instance(), SIGNAL(saveSettingsRequested()),
        this, SLOT(savePersistentSettings()));


    d->m_AssetExplorerSettings.environmentId =
            QUuid(s->value(QLatin1String("AssetExplorer/Settings/EnvironmentId")).toByteArray());
    if (d->m_AssetExplorerSettings.environmentId.isNull())
        d->m_AssetExplorerSettings.environmentId = QUuid::createUuid();



    updateActions();
#endif
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


void AssetExplorer::setCurrent(Project* pProject, const QString& name, Node* pNode)
{
	X_UNUSED(pProject);
	X_UNUSED(name);
	X_UNUSED(pNode);

    if (debugLogging) {
        qDebug() << "ProjectExplorer - setting path to " << (name)
                << " and project to " << (pProject ? pProject->displayName() : QLatin1String("0"));
    }


	X_ASSERT_NOT_IMPLEMENTED();
}


void AssetExplorer::showContextMenu(QWidget* pView, const QPoint& globalPos, Node* pNode)
{
    qDebug() << "AssetExplorer::showContextMenu";
	X_UNUSED(pView);
	X_UNUSED(globalPos);
	X_UNUSED(pNode);


	X_ASSERT_NOT_IMPLEMENTED();
}


void AssetExplorer::renameFile(Node* pNode, const QString& to)
{
	X_ASSERT_NOT_IMPLEMENTED();
}

void AssetExplorer::updateActions(void)
{
    qDebug() << "AssetExplorer::updateActions";

}

void AssetExplorer::updateContextMenuActions(void)
{
    qDebug() << "AssetExplorer::updateContextMenuActions";

	X_ASSERT_NOT_IMPLEMENTED();
}



void AssetExplorer::projectAdded(Project* pPro)
{
    connect(pPro, SIGNAL(buildConfigurationEnabledChanged()),
            this, SLOT(updateActions()));
}

void AssetExplorer::projectRemoved(Project* pPro)
{
    disconnect(pPro, SIGNAL(buildConfigurationEnabledChanged()),
               this, SLOT(updateActions()));
}

void AssetExplorer::projectDisplayNameChanged(Project* pPro)
{
	X_UNUSED(pPro);
    updateActions();
}


void AssetExplorer::newProject(void)
{
    qDebug() << "AssetExplorer::newProject";


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




} // namespace AssetExplorer

X_NAMESPACE_END
