#ifndef SESSION_H
#define SESSION_H



#include <QObject>

X_NAMESPACE_BEGIN(assman)


namespace AssetExplorer
{

	class Project;
	class Node;
	class SessionNode;


	class SessionManager : public QObject
	{
		Q_OBJECT
	public:
		explicit SessionManager(QObject* pParent = nullptr);
		~SessionManager();

		static QObject* instance(void);

		// higher level session management
		static QString activeSession(void);
		static QString lastSession(void);
		static QStringList sessions(void);

		// Projects are mostlys Mod's but they are named projects as I might add use thm for others things than mods.
		static void addProject(Project* pProject);
		static void addProjects(const QList<Project*>& projects);
		static void removeProject(Project* pProject);
		static void removeProjects(QList<Project*> remove);

		static void setStartupProject(Project* pStartupProject);

		static SessionNode* sessionNode(void);
		static Project* startupProject(void);

		static const QList<Project*>& projects(void);
		static bool hasProjects(void);

		static bool isDefaultVirgin(void);
		static bool isDefaultSession(const QString& session);

		static Project* projectForNode(Node* pNode);

	signals:
		void projectAdded(Project* pProject);
		void singleProjectAdded(Project* pProject);
		void aboutToRemoveProject(Project* pProject);
		void projectDisplayNameChanged(Project* pProject);
		void projectRemoved(Project* pProject);
		void startupProjectChanged(Project* pProject);

	private slots:
		static void projectDisplayNameChanged(void);
	};

} // namespace AssetExplorer

X_NAMESPACE_END

#endif // SESSION_H
