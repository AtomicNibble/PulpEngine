#include "assetdbwidget.h"

#include "assetdbnodes.h"
#include "assetdbmodel.h"
#include "assetdbexplorer.h"
#include "assetdbconstants.h"

#include "session.h"
#include "project.h"

#include "logging.h"

#include <QtWidgets>
#include <QHeaderView>
#include <QFocusEvent>
#include <QAbstractItemModel>

X_NAMESPACE_BEGIN(assman)

namespace AssetExplorer
{

namespace
{

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

} // namespace

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





AssetDbViewWidget::AssetDbViewWidget(AssetDB& db, QWidget *parent) :
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

    NodesWatcher* pWatcher = new NodesWatcher(this);
    SessionManager::sessionNode()->registerWatcher(pWatcher);

    connect(pWatcher, SIGNAL(foldersAboutToBeRemoved(FolderNode*,QList<FolderNode*>)),
            this, SLOT(foldersAboutToBeRemoved(FolderNode*,QList<FolderNode*>)));
    connect(pWatcher, SIGNAL(filesAboutToBeRemoved(FolderNode*,QList<FileNode*>)),
            this, SLOT(filesAboutToBeRemoved(FolderNode*,QList<FileNode*>)));

	QObject* pSessionManager = SessionManager::instance();
	connect(pSessionManager, SIGNAL(singleProjectAdded(Project*)),
		this, SLOT(handleProjectAdded(Project*)));
	connect(pSessionManager, SIGNAL(startupProjectChanged(Project*)),
		this, SLOT(startupProjectChanged(Project*)));

    search_ = new QLineEdit(this);
    search_->setPlaceholderText("Search..");

    view_ = new AssetDbTreeView(this);
    view_->setModel(model_);
  //  view_->setItemDelegate(new AssetDbTreeItemDelegate(this));
    setFocusProxy(view_);
    initView();

    model_->setTreeView(view_);


	connect(model_, SIGNAL(modelReset()),
		this, SLOT(initView()));
	connect(view_, SIGNAL(activated(QModelIndex)),
		this, SLOT(openItem(QModelIndex)));
	connect(view_, SIGNAL(customContextMenuRequested(QPoint)),
		this, SLOT(showContextMenu(QPoint)));


	// make whole widget have assDB context, not just the tree.
	pContext_ = new IContext(this);
	pContext_->setContext(Context(assman::AssetExplorer::Constants::C_ASSETDB_EXPLORER));
	pContext_->setWidget(this);
	ICore::addContextObject(pContext_);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(view_);
    mainLayout->addWidget(search_);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setMargin(0);
    setLayout(mainLayout);

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

void AssetDbViewWidget::setCurrentItem(Node* pNode, Project* pProject)
{
   qCDebug(logCatAssetDbView) << "ProjectTreeWidget::setCurrentItem(" << (pProject ? pProject->displayName() : QLatin1String("0"))
          << ", " <<  (pNode ? pNode->name() : QLatin1String("0")) << ")";

    if (!pProject) {
        return;
    }

    const QModelIndex mainIndex = model_->indexForNode(pNode);

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
    Node* pNode = model_->nodeForIndex(index);
    explorer_->showContextMenu(this, view_->mapToGlobal(pos), pNode);
}

void AssetDbViewWidget::collapseAll(void)
{
    view_->collapseAll();
}

void AssetDbViewWidget::editCurrentItem(void)
{
    if (view_->selectionModel()->currentIndex().isValid()) {
        view_->edit(view_->selectionModel()->currentIndex());
    }
}


void AssetDbViewWidget::handleProjectAdded(Project *pProject)
{
     Node* pNode = pProject->rootProjectNode();
     QModelIndex idx = model_->indexForNode(pNode);
	 if (autoExpand_) {// disabled while session restoring
		 view_->setExpanded(idx, true);
	 }
    view_->setCurrentIndex(idx);
}


void AssetDbViewWidget::startupProjectChanged(Project* pProject)
{
    if (pProject) {
        ProjectNode* pNode = pProject->rootProjectNode();
        model_->setStartupProject(pNode);
    } else {
        model_->setStartupProject(nullptr);
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

X_NAMESPACE_END
