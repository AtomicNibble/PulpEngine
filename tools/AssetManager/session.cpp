#include "session.h"
#include "assetdbnodes.h"

#include "project.h"
#include "assetdbexplorer.h"

#include <QStringList>
#include <QDir>
#include <QMap>

X_NAMESPACE_BEGIN(assman)

namespace AssetExplorer 
{

namespace {
bool debug = false;
}


class SessionManagerPrivate
{
public:
    SessionManagerPrivate() :
        sessionName_(QLatin1String("default")),
        virginSession_(true),
        startupProject_(nullptr)
    {}

    bool projectContainsFile(Project *p, const QString &fileName) const;

public:
    SessionNode *sessionNode_;
    QString sessionName_;
    bool virginSession_;

    mutable QStringList sessions_;

    Project *startupProject_;
    QList<Project *> projects_;
};


static SessionManager *instance_ = 0;
static SessionManagerPrivate *d = 0;


SessionManager::SessionManager(QObject *parent)
  : QObject(parent)
{
    instance_ = this;
    d = new SessionManagerPrivate;
    d->sessionNode_ = new SessionNode(this);

}


SessionManager::~SessionManager()
{
    delete d;
}

QObject *SessionManager::instance()
{
   return instance_;
}


bool SessionManager::isDefaultVirgin()
{
    return isDefaultSession(d->sessionName_) && d->virginSession_;
}

bool SessionManager::isDefaultSession(const QString &session)
{
    return session == QLatin1String("default");
}


void SessionManager::setStartupProject(Project *startupProject)
{
    qDebug() << Q_FUNC_INFO << (startupProject ? startupProject->displayName() : QLatin1String("0"));

    if (startupProject) {
        Q_ASSERT(d->projects_.contains(startupProject));
    }

    if (d->startupProject_ == startupProject) {
        return;
    }

    d->startupProject_ = startupProject;
}

Project *SessionManager::startupProject()
{
    return d->startupProject_;
}

void SessionManager::addProject(Project *project)
{
    addProjects(QList<Project*>() << project);
}

void SessionManager::addProjects(const QList<Project*> &projects)
{
    d->virginSession_ = false;
    QList<Project*> clearedList;
	for (Project* pPro : projects) {
        if (!d->projects_.contains(pPro)) {
            clearedList.append(pPro);
            d->projects_.append(pPro);
            d->sessionNode_->addProjectNodes(QList<ProjectNode*>() << pPro->rootProjectNode());

            connect(pPro, SIGNAL(displayNameChanged()),
                    instance_, SLOT(projectDisplayNameChanged()));

            qDebug() << "SessionManager - adding project " << pPro->displayName();
        }
    }
}

void SessionManager::removeProject(Project *project)
{
    d->virginSession_ = false;
    if (project == 0) {
        qDebug() << "SessionManager::removeProject(0)";
        return;
    }
    removeProjects(QList<Project*>() << project);
}


void SessionManager::removeProjects(QList<Project *> remove)
{

}

const QList<Project *> &SessionManager::projects()
{
    return d->projects_;
}

bool SessionManager::hasProjects()
{
    return !d->projects_.isEmpty();
}


QString SessionManager::activeSession()
{
    return d->sessionName_;
}

QStringList SessionManager::sessions()
{
    return d->sessions_;
}


QString SessionManager::lastSession()
{
    return "game"; // ICore::settings()->value(QLatin1String("ProjectExplorer/StartupSession")).toString();
}


SessionNode *SessionManager::sessionNode()
{
    return d->sessionNode_;
}



void SessionManager::projectDisplayNameChanged()
{
    Project* pPro = qobject_cast<Project*>(instance_->sender());
    if (pPro) {

        QList<ProjectNode*> nodes;
        nodes << pPro->rootProjectNode();
        d->sessionNode_->removeProjectNodes(nodes);
        d->sessionNode_->addProjectNodes(nodes);

        emit instance_->projectDisplayNameChanged(pPro);
    }
}


Project *SessionManager::projectForNode(Node *node)
{
    if (!node) {
        return nullptr;
    }

    FolderNode* pRootProjectNode = qobject_cast<FolderNode*>(node);
    if (!pRootProjectNode) {
		pRootProjectNode = node->parentFolderNode();
    }

    while (pRootProjectNode && pRootProjectNode->parentFolderNode() != d->sessionNode_) {
		pRootProjectNode = pRootProjectNode->parentFolderNode();
    }

    Q_ASSERT(pRootProjectNode);

    for (Project* pProject : d->projects_) {
        if (pProject->rootProjectNode() == pRootProjectNode) {
            return pProject;
        }
    }

    return nullptr;
}



} // namespace AssetExplorer

X_NAMESPACE_END
