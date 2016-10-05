#include "session.h"
#include "assetdbnodes.h"

#include "project.h"
#include "assetdbexplorer.h"


X_NAMESPACE_BEGIN(assman)

namespace AssetExplorer 
{


class SessionManagerPrivate
{
public:
    SessionManagerPrivate() :
        sessionName_(QLatin1String("default")),
        virginSession_(true),
		pSessionNode_(nullptr),
		pStartupProject_(nullptr)
    {
	}

public:
    SessionNode* pSessionNode_;
    QString sessionName_;
    bool virginSession_;

    mutable QStringList sessions_;

    Project* pStartupProject_;
    QList<Project*> projects_;
	QMap<QString, QVariant> values_;
};


static SessionManager* instance_ = nullptr;
static SessionManagerPrivate* d = nullptr;


SessionManager::SessionManager(QObject* pParent)
  : QObject(pParent)
{
    instance_ = this;
    d = new SessionManagerPrivate;
    d->pSessionNode_ = new SessionNode(this);
}


SessionManager::~SessionManager()
{
	delete d;
}

QObject* SessionManager::instance(void)
{
	X_ASSERT_NOT_NULL(instance_);
	return instance_;
}

void SessionManager::setValue(const QString& name, const QVariant& value)
{
	if (d->values_.value(name) == value) {
		return;
	}
	d->values_.insert(name, value);
}

QVariant SessionManager::value(const QString& name)
{
	auto it = d->values_.constFind(name);
	return (it == d->values_.constEnd()) ? QVariant() : *it;
}


bool SessionManager::isDefaultVirgin(void)
{
    return isDefaultSession(d->sessionName_) && d->virginSession_;
}

bool SessionManager::isDefaultSession(const QString& session)
{
    return session == QLatin1String("default");
}


void SessionManager::setStartupProject(Project* pStartupProject)
{
    qDebug() << Q_FUNC_INFO << (pStartupProject ? pStartupProject->displayName() : QLatin1String("0"));

    if (pStartupProject) {
        X_ASSERT(d->projects_.contains(pStartupProject), "Attempted to set startup project to a instance that is not in the projects list")();
    }

    if (d->pStartupProject_ == pStartupProject) {
        return;
    }

    d->pStartupProject_ = pStartupProject;

	// used by assetDb widget and shit :D
	emit instance_->startupProjectChanged(pStartupProject);
}

Project* SessionManager::startupProject(void)
{
    return d->pStartupProject_;
}

void SessionManager::addProject(Project* pProject)
{
    addProjects(QList<Project*>() << pProject);
}

void SessionManager::addProjects(const QList<Project*>& projects)
{
    d->virginSession_ = false;
    QList<Project*> clearedList;
	for (Project* pPro : projects) 
	{
        if (!d->projects_.contains(pPro)) 
		{
            clearedList.append(pPro);
            d->projects_.append(pPro);
            d->pSessionNode_->addProjectNodes(QList<ProjectNode*>() << pPro->rootProjectNode());

            connect(pPro, SIGNAL(displayNameChanged()),
                    instance_, SLOT(projectDisplayNameChanged()));

            qDebug() << "SessionManager - adding project " << pPro->displayName();
        }
    }
}

void SessionManager::removeProject(Project* pProject)
{
    d->virginSession_ = false;
    if (pProject == nullptr) {
        qDebug() << "SessionManager::removeProject(0)";
        return;
    }
    removeProjects(QList<Project*>() << pProject);
}


void SessionManager::removeProjects(QList<Project*> remove)
{
	X_ASSERT_NOT_IMPLEMENTED();
}

const QList<Project *> &SessionManager::projects(void)
{
    return d->projects_;
}

bool SessionManager::hasProjects(void)
{
    return !d->projects_.isEmpty();
}

QString SessionManager::activeSession(void)
{
    return d->sessionName_;
}

QStringList SessionManager::sessions(void)
{
    return d->sessions_;
}

QString SessionManager::lastSession(void)
{
    return "game"; // ICore::settings()->value(QLatin1String("ProjectExplorer/StartupSession")).toString();
}


SessionNode* SessionManager::sessionNode(void)
{
    return d->pSessionNode_;
}

void SessionManager::projectDisplayNameChanged(void)
{
    Project* pPro = qobject_cast<Project*>(instance_->sender());
    if (pPro) {

        QList<ProjectNode*> nodes;
        nodes << pPro->rootProjectNode();
        d->pSessionNode_->removeProjectNodes(nodes);
        d->pSessionNode_->addProjectNodes(nodes);

        emit instance_->projectDisplayNameChanged(pPro);
    }
}


Project* SessionManager::projectForNode(Node* pNode)
{
    if (!pNode) {
        return nullptr;
    }

    FolderNode* pRootProjectNode = qobject_cast<FolderNode*>(pNode);
    if (!pRootProjectNode) {
		pRootProjectNode = pNode->parentFolderNode();
    }

    while (pRootProjectNode && pRootProjectNode->parentFolderNode() != d->pSessionNode_) {
		pRootProjectNode = pRootProjectNode->parentFolderNode();
    }

	X_ASSERT_NOT_NULL(pRootProjectNode);

    for (Project* pProject : d->projects_) {
        if (pProject->rootProjectNode() == pRootProjectNode) {
            return pProject;
        }
    }

    return nullptr;
}



} // namespace AssetExplorer

X_NAMESPACE_END
