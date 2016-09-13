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

X_NAMESPACE_BEGIN(assman)

namespace AssetExplorer {

class Project;
class Node;
class FolderNode;


class AssetExplorer : public QObject
{
    Q_OBJECT
public:
    AssetExplorer(assetDb::AssetDB& db);
    ~AssetExplorer();

    static AssetExplorer* instance(void);

    bool init(void);
	bool loadMods(void);
	bool addMod(int32_t modId, const core::string& name, core::Path<char>& outDir);

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

    void updateActions(void);

private:
    void setCurrent(Project* pProject, const QString& filePath, Node* pNode);
    void updateContextMenuActions(void);

private:
    QMenu* projectMenu_;
    QMenu* folderMenu_;
    QMenu* fileMenu_;

    QAction* addNewFileAction_;
    QAction* addExistingFilesAction_;
    QAction* removeFileAction_;
    QAction* deleteFileAction_;
    QAction* renameFileAction_;
    QAction* openFileAction_;
    QAction* openContaingFolderAction_;
    QAction* projectTreeCollapseAllAction_;
    QAction* unloadAction_;

private:
	assetDb::AssetDB& db_;
    Project* currentProject_;
    Node* currentNode_;

    static AssetExplorer *instance_;
};


} // namespace AssetExplorer

X_NAMESPACE_END

#endif // ASSETDBEXPLORER_H
