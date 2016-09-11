#ifndef ASSETDBEXPLORER_H
#define ASSETDBEXPLORER_H

#include <QObject>
#include <QStringList>

class QPoint;
class QMenu;
class QAction;


namespace AssetExplorer {

class Project;
class Node;
class FolderNode;


class AssetExplorer : public QObject
{
    Q_OBJECT
public:
    AssetExplorer();
    ~AssetExplorer();

    static AssetExplorer* instance(void);

    bool init(QString* errorMessage);

    static Project* currentProject(void);
    Node *currentNode(void) const;
    void setCurrentNode(Node* node);

    void renameFile(Node* pNode, const QString &to);

    void showContextMenu(QWidget* view, const QPoint &globalPos, Node* pNode);

signals:

    void aboutToShowContextMenu(Project* pProject, Node* pNode);

    // Is emitted when a project has been added/removed,
    // or the file list of a specific project has changed.
    void fileListChanged(void);

    void currentProjectChanged(Project* pProject);
    void currentNodeChanged(Node* pNode, Project* pProject);

    void recentProjectsChanged(void);

public slots:

private slots:
    void setStartupProject(void);
    void setStartupProject(Project* pProject);
    void newProject(void);

    void projectAdded(Project* pProject);
    void projectRemoved(Project* pProject);
    void projectDisplayNameChanged(Project* pProject);
    void startupProjectChanged(void); // Calls updateRunAction

    void updateActions(void);

private:
    void setCurrent(Project* pProject, QString filePath, Node* pNode);

    void updateContextMenuActions(void);

    void addToRecentProjects(const QString& fileName, const QString& displayName);

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
    Project* currentProject_;
    Node* currentNode_;

    static AssetExplorer *instance_;
};



} // namespace AssetExplorer


#endif // ASSETDBEXPLORER_H
