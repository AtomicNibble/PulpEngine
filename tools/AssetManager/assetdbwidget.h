#ifndef ASSETDBWIDGET_H
#define ASSETDBWIDGET_H


#include <QWidget>
#include <QTreeView>

class QLineEdit;
class AssetDb;

namespace AssetExplorer
{

class AssetDbTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit AssetDbTreeView(QWidget *parent = 0);

protected:
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
 //   IContext *m_context;
};

class Project;
class Node;
class FolderNode;
class FileNode;

class AssetExplorer;
class AssetDBModel;


class AssetDbViewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AssetDbViewWidget(AssetDb& db, QWidget *parent = 0);

signals:

public slots:
    void collapseAll();
    void editCurrentItem();

private slots:
    void initView();

    void showContextMenu(const QPoint &pos);

    void setCurrentItem(Node *node, Project *project);
    void openItem(const QModelIndex &mainIndex);
    void handleProjectAdded(Project *project);
    void startupProjectChanged(Project *project);

    void foldersAboutToBeRemoved(FolderNode*, const QList<FolderNode*> &);
    void filesAboutToBeRemoved(FolderNode*, const QList<FileNode*> &);

    void disableAutoExpand();

private:
    AssetExplorer* explorer_;
    AssetDBModel* model_;
    AssetDbTreeView*  view_;

    QLineEdit* search_;

    bool autoExpand_;
};


} // namespace AssetExplorer


#endif // ASSETDBWIDGET_H
