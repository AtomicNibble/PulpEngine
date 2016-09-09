#ifndef SESSION_H
#define SESSION_H



#include <QObject>



namespace AssetExplorer {

class Project;
class Node;
class SessionNode;


class SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(QObject *parent = 0);
    ~SessionManager();

    static QObject *instance();

    // higher level session management
    static QString activeSession();
    static QString lastSession();
    static QStringList sessions();

    // Projects are mostlys Mod's but they are named projects as I might add use thm for others things than mods.
    static void addProject(Project *project);
    static void addProjects(const QList<Project*> &projects);
    static void removeProject(Project *project);
    static void removeProjects(QList<Project *> remove);

    static void setStartupProject(Project *startupProject);

    static SessionNode *sessionNode();
    static Project *startupProject();

    static const QList<Project *> &projects();
    static bool hasProjects();

    static bool isDefaultVirgin();
    static bool isDefaultSession(const QString &session);

    static Project *projectForNode(Node *node);

signals:
    void projectAdded(Project *project);
    void singleProjectAdded(Project *project);
    void aboutToRemoveProject(Project *project);
    void projectDisplayNameChanged(Project *project);
    void projectRemoved(Project *project);
    void startupProjectChanged(Project* project);


private slots:
    static void projectDisplayNameChanged();
};

} // namespace AssetExplorer

#endif // SESSION_H
