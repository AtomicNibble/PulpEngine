#include "assetdbwidget.h"

#include "assetdbnodes.h"
#include "assetdbmodel.h"
#include "assetdbexplorer.h"

#include "session.h"
#include "project.h"

#include "logging.h"

#include <QtWidgets>
#include <QHeaderView>
#include <QFocusEvent>
#include <QAbstractItemModel>


namespace AssetExplorer
{

namespace {

class AssetDbTreeItemDelegate : public QStyledItemDelegate
{
public:
    AssetDbTreeItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
    { }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyleOptionViewItem opt = option;
      //  if (!index.data(ProjectExplorer::Project::EnabledRole).toBool())
            opt.state &= ~QStyle::State_Enabled;
        QStyledItemDelegate::paint(painter, opt, index);
    }
};

}

AssetDbTreeView::AssetDbTreeView(QWidget *parent)
    : QTreeView(parent)
{
    setFrameStyle(QFrame::NoFrame);
    setIndentation(indentation() * 9/10);
    setUniformRowHeights(true);
    setTextElideMode(Qt::ElideNone);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    setHeaderHidden(true);

    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setContextMenuPolicy(Qt::CustomContextMenu);

    // We let the column adjust to contents, but note
    // the setting of a minimum size in resizeEvent()
    QHeaderView * head = header();

    head->setSectionResizeMode(QHeaderView::ResizeToContents);
    head->setStretchLastSection(false);
}

void AssetDbTreeView::focusInEvent(QFocusEvent *event)
{
    if (event->reason() != Qt::PopupFocusReason) {
        QTreeView::focusInEvent(event);
    }
}

void AssetDbTreeView::focusOutEvent(QFocusEvent *event)
{
    if (event->reason() != Qt::PopupFocusReason) {
        QTreeView::focusOutEvent(event);
    }
}

void AssetDbTreeView::resizeEvent(QResizeEvent *event)
{
    header()->setMinimumSectionSize(viewport()->width());
    QTreeView::resizeEvent(event);
}



// ------------------------------------------------------------------





AssetDbViewWidget::AssetDbViewWidget(AssetDb& db, QWidget *parent) :
    QWidget(parent),
    explorer_(AssetExplorer::instance()),
    model_(nullptr),
    view_(nullptr),
    search_(nullptr),
    autoExpand_(true)
{
    model_ = new AssetDBModel(SessionManager::sessionNode(), db, this);

  //  Project *pro = SessionManager::startupProject();
  //  if (pro)
   //     model_->setStartupProject(pro->rootProjectNode());

    NodesWatcher *watcher = new NodesWatcher(this);
    SessionManager::sessionNode()->registerWatcher(watcher);

    connect(watcher, SIGNAL(foldersAboutToBeRemoved(FolderNode*,QList<FolderNode*>)),
            this, SLOT(foldersAboutToBeRemoved(FolderNode*,QList<FolderNode*>)));
    connect(watcher, SIGNAL(filesAboutToBeRemoved(FolderNode*,QList<FileNode*>)),
            this, SLOT(filesAboutToBeRemoved(FolderNode*,QList<FileNode*>)));


    search_ = new QLineEdit(this);
    search_->setPlaceholderText("Search..");

    view_ = new AssetDbTreeView(this);
    view_->setModel(model_);
    view_->setItemDelegate(new AssetDbTreeItemDelegate(this));
    setFocusProxy(view_);
    initView();

    model_->setTreeView(view_);


    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(view_);
    mainLayout->addWidget(search_);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setMargin(0);

    setLayout(mainLayout);


    connect(model_, SIGNAL(modelReset()),
            this, SLOT(initView()));
    connect(view_, SIGNAL(activated(QModelIndex)),
            this, SLOT(openItem(QModelIndex)));
    connect(view_, SIGNAL(customContextMenuRequested(QPoint)),
        this, SLOT(showContextMenu(QPoint)));

}


void AssetDbViewWidget::initView()
{
    QModelIndex sessionIndex = model_->index(0, 0);

    // hide root folder
    view_->setRootIndex(sessionIndex);

 //   while (model_->canFetchMore(sessionIndex)) {
  //      model_->fetchMore(sessionIndex);
  //  }

    // expand top level projects
 //   for (int i = 0; i < model_->rowCount(sessionIndex); ++i)
 //      view_->expand(model_->index(i, 0, sessionIndex));

    setCurrentItem(explorer_->currentNode(), AssetExplorer::currentProject());

    view_->setStyleSheet(
        "border: 1px solid #3F3F46;"
        "border-top: none;"
    );
}

void AssetDbViewWidget::setCurrentItem(Node *node, Project *project)
{
   qCDebug(logCatAssetDbView) << "ProjectTreeWidget::setCurrentItem(" << (project ? project->displayName() : QLatin1String("0"))
          << ", " <<  (node ? node->name() : QLatin1String("0")) << ")";

    if (!project) {
        return;
    }

    const QModelIndex mainIndex = model_->indexForNode(node);

    if (mainIndex.isValid()) {
        if (mainIndex != view_->selectionModel()->currentIndex()) {
            view_->setCurrentIndex(mainIndex);
            view_->scrollTo(mainIndex);
        }
    } else {
        qCDebug(logCatAssetDbView) << "clear selection";
        view_->clearSelection();
    }
}

void AssetDbViewWidget::showContextMenu(const QPoint &pos)
{
    QModelIndex index = view_->indexAt(pos);
    Node *node = model_->nodeForIndex(index);
 //   explorer_->showContextMenu(this, m_view->mapToGlobal(pos), node);
}

void AssetDbViewWidget::collapseAll()
{
    view_->collapseAll();
}

void AssetDbViewWidget::editCurrentItem()
{
    if (view_->selectionModel()->currentIndex().isValid()) {
        view_->edit(view_->selectionModel()->currentIndex());
    }
}


void AssetDbViewWidget::handleProjectAdded(Project *project)
{
     Node *node = project->rootProjectNode();
     QModelIndex idx = model_->indexForNode(node);
     if (autoExpand_) // disabled while session restoring
        view_->setExpanded(idx, true);
    view_->setCurrentIndex(idx);
}


void AssetDbViewWidget::startupProjectChanged(Project *project)
{
    if (project) {
        ProjectNode *node = project->rootProjectNode();
        model_->setStartupProject(node);
    } else {
        model_->setStartupProject(0);
    }
}



void AssetDbViewWidget::openItem(const QModelIndex &mainIndex)
{
    Node *node = model_->nodeForIndex(mainIndex);
    if (node->nodeType() != NodeType::FileNodeType) {
        return;
    }

    /*
    IEditor *editor = EditorManager::openEditor(node->path());
    if (editor && node->line() >= 0)
        editor->gotoLine(node->line());
*/
    qDebug() << "Open file: " << node->name();
}


void AssetDbViewWidget::foldersAboutToBeRemoved(FolderNode *, const QList<FolderNode*> &list)
{
    Node *n = explorer_->currentNode();
    while (n) {
        if (FolderNode *fn = qobject_cast<FolderNode *>(n)) {
            if (list.contains(fn)) {
                ProjectNode *pn = n->projectNode();
                // Make sure the node we are switching too isn't going to be removed also
                while (list.contains(pn)) {
                    pn = pn->parentFolderNode()->projectNode();
                }
                explorer_->setCurrentNode(pn);
                break;
            }
        }
        n = n->parentFolderNode();
    }
}

void AssetDbViewWidget::filesAboutToBeRemoved(FolderNode *, const QList<FileNode*> &list)
{
    if (FileNode *fileNode = qobject_cast<FileNode *>(explorer_->currentNode())) {
        if (list.contains(fileNode)) {
            explorer_->setCurrentNode(fileNode->projectNode());
        }
    }
}


void AssetDbViewWidget::disableAutoExpand()
{
    autoExpand_ = false;
}






} // namespace AssetExplorer
