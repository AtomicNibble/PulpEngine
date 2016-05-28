#include "session.h"
#include "assetdbnodes.h"

#include "project.h"

#include <QStringList>
#include <QDir>
#include <QMap>

namespace AssetExplorer {


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

    mutable QStringList m_sessions;

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
    emit instance_->aboutToUnloadSession(d->sessionName_);

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
    emit instance_->startupProjectChanged(startupProject);
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
    foreach (Project *pro, projects) {
        if (!d->projects_.contains(pro)) {
            clearedList.append(pro);
            d->projects_.append(pro);
            d->sessionNode_->addProjectNodes(QList<ProjectNode *>() << pro->rootProjectNode());

            connect(pro, SIGNAL(fileListChanged()),
                    instance_, SLOT(clearProjectFileCache()));

            connect(pro, SIGNAL(displayNameChanged()),
                    instance_, SLOT(projectDisplayNameChanged()));


            qDebug() << "SessionManager - adding project " << pro->displayName();
        }
    }

    foreach (Project *pro, clearedList) {
        emit instance_->projectAdded(pro);
    }

    if (clearedList.count() == 1) {
        emit instance_->singleProjectAdded(clearedList.first());
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

bool SessionManager::save()
{
    qDebug() << "SessionManager - saving session" << d->sessionName_;

    emit instance_->aboutToSaveSession();

    bool result = true;


   qDebug() << "SessionManager - saving session returned " << result;

    return result;
}

void SessionManager::closeAllProjects()
{
  setStartupProject(0);
  removeProjects(projects());
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
    return d->m_sessions;
}



bool SessionManager::createSession(const QString &session)
{
    if (sessions().contains(session))
        return false;
    Q_ASSERT(d->m_sessions.size() > 0);
    d->m_sessions.insert(1, session);
    return true;
}



bool SessionManager::loadSession(const QString &session)
{
    if (session == d->sessionName_ && !isDefaultVirgin())
        return true;

    if (!sessions().contains(session))
        return false;


    // Allow everyone to set something in the session and before saving
    emit instance_->aboutToUnloadSession(d->sessionName_);


    return true;
}

QString SessionManager::lastSession()
{
    return "game"; // ICore::settings()->value(QLatin1String("ProjectExplorer/StartupSession")).toString();
}


SessionNode *SessionManager::sessionNode()
{
    return d->sessionNode_;
}



} // namespace AssetExplorer
