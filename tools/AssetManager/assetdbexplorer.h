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

    static AssetExplorer *instance(void);

    bool init(QString *errorMessage);

    Project *openProject(const QString &fileName);
    Project *openProject(const QString &fileName, QString *error);
    QList<Project *> openProjects(const QStringList &fileNames, QString *error);
    void unloadProject(Project *project);


    static Project *currentProject(void);
    Node *currentNode(void) const;

    void renameFile(Node *node, const QString &to);

    void showContextMenu(QWidget *view, const QPoint &globalPos, Node *node);

signals:

    void aboutToShowContextMenu(Project *project, Node *node);

    // Is emitted when a project has been added/removed,
    // or the file list of a specific project has changed.
    void fileListChanged(void);

    void currentProjectChanged(Project *project);
    void currentNodeChanged(Node *node, Project *project);

    void recentProjectsChanged(void);

public slots:

private slots:
    void setStartupProject(void);
    void setStartupProject(Project *project);
    void unloadProject(void);
    void newProject(void);

    void projectAdded(Project *pro);
    void projectRemoved(Project *pro);
    void projectDisplayNameChanged(Project *pro);
    void startupProjectChanged(void); // Calls updateRunAction

    void updateActions(void);

private:
    void updateContextMenuActions(void);

    void addToRecentProjects(const QString &fileName, const QString &displayName);

private:
    QMenu *m_projectMenu;
    QMenu *m_folderMenu;
    QMenu *m_fileMenu;

    QAction* m_addNewFileAction;
    QAction* m_addExistingFilesAction;
    QAction *m_removeFileAction;
    QAction *m_deleteFileAction;
    QAction *m_renameFileAction;
    QAction *m_openFileAction;
    QAction *m_openContaingFolderAction;
    QAction *m_projectTreeCollapseAllAction;
    QAction *m_unloadAction;

private:
    Project* currentProject_;
    Node* m_currentNode;

    static AssetExplorer *instance_;
};



} // namespace AssetExplorer


#endif // ASSETDBEXPLORER_H
