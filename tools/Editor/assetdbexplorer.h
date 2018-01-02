#ifndef ASSETDBEXPLORER_H
#define ASSETDBEXPLORER_H

#include <QObject>
#include <QStringList>

class QPoint;
class QMenu;
class QAction;


X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(editor)

class ParameterAction;

namespace AssetExplorer {

	class Project;
	class Node;
	class FileNode;
	class FolderNode;

	class AssetExplorer : public QObject
	{
		Q_OBJECT
	public:
		AssetExplorer(assetDb::AssetDB& db);
		~AssetExplorer();

		static AssetExplorer* instance(void);

		bool init(void);
		bool delayedInit(void);
		bool loadMods(void);
		bool restoreSession(void);
		bool addMod(int32_t modId, const core::string& name, const core::Path<char>& outDir);

		static Project* currentProject(void);
		Node* currentNode(void) const;
		void setCurrentNode(Node* node);

		void showContextMenu(QWidget* view, const QPoint& globalPos, Node* pNode);

		void renameFile(Node* pNode, const QString& to);

	signals:

		void aboutToShowContextMenu(Project* pProject, Node* pNode);

		// Is emitted when a project has been added/removed,
		// or the file list of a specific project has changed.	
		void fileListChanged(void);

		void currentProjectChanged(Project* pProject);
		void currentNodeChanged(Node* pNode, Project* pProject);
		void recentProjectsChanged(void);


	private slots:
		void setStartupProject(void);
		void setStartupProject(Project* pProject);
		void newProject(void);

		void projectAdded(Project* pProject);
		void projectRemoved(Project* pProject);
		void projectDisplayNameChanged(Project* pProject);
		void startupProjectChanged(void);

		void openAsset(void);
		void renameAsset(void);
		void deleteAsset(void);
		void cutAsset(void);
		void copyAsset(void);
		void pasteAsset(void);
		void copyAssetName(void);

		void addNewAsset(void);
		void addNewAssetType(void);

		void build(void);
		void buildForce(void);
		void cleanMod(void);

		void updateActions(void);

		void savePersistentSettings(void);

	private:
		void setCurrent(Project* pProject, const QString& filePath, Node* pNode);
		void updateContextMenuActions(void);

	private:
		QMenu* sessionMenu_;
		QMenu* projectMenu_;
		QMenu* folderMenu_;
		QMenu* fileMenu_;

		// actions for a Asset
		QAction* openAssetAction_;
		QAction* renameAssetAction_;
		QAction* deleteAssetAction_;
		QAction* cutAssetAction_;
		QAction* copyAssetAction_;
		QAction* pasteAssetAction_;
		QAction* copyAssetNameAction_;


		QAction* addNewAssetAction_;
		QAction* addNewAssetTypeAction_;
		QAction* projectTreeCollapseAllAction_;
		QAction* projectTreeExpandAllAction_;
		QAction* projectTreeExpandBelowAction_;
		QAction* buildAction_;
		QAction* reBuildAction_;
		QAction* cleanModAction_;

		ParameterAction* setStartupModAction_;

	private:
		assetDb::AssetDB& db_;
		Project* currentProject_;
		Node* currentNode_;

		static AssetExplorer *instance_;
	};


} // namespace AssetExplorer

X_NAMESPACE_END

#endif // ASSETDBEXPLORER_H
