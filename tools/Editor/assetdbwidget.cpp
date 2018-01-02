#include "assetdbwidget.h"

#include "assetdbnodes.h"
#include "assetdbmodel.h"
#include "assetdbexplorer.h"
#include "assetdbconstants.h"

#include "EditorManager.h"

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

void AssetDbTreeView::expandRecursive(const QModelIndex &index)
{
	if (!index.isValid()) {
		return;
	}

	const int32_t childCount = index.model()->rowCount(index);
	for (int32_t i = 0; i < childCount; i++) {
		const QModelIndex &child = index.child(i, 0);
		expandRecursive(child);
	}

	if (!isExpanded(index)) {
		expand(index);
	}
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

AssetDbFilterOptionsWidget::AssetDbFilterOptionsWidget(QWidget* parent) :
	QWidget(parent)
{
	core::zero_object(typesEnabled_);

	for (uint32_t i = 0; i < assetDb::AssetType::ENUM_COUNT; i++)
	{
		const char* pName = assetDb::AssetType::ToString(i);
		QString name(pName);

		typeNamesCache_[i] = name.toLower();
	}

	pNameFilterEnabled_ = new QCheckBox();
	pNameFilterEnabled_->setText("Name");
	pNameFilterEnabled_->setChecked(true);
	pTypeFilterEnabled_ = new QCheckBox();
	pTypeFilterEnabled_->setText("Types");
	pTypeFilterEnabled_->setChecked(true);

	pNameFilter_ = new QLineEdit();
	pNameFilter_->setPlaceholderText("Search..");

	pTypesFilter_ = new QLineEdit();
	pTypesFilter_->setPlaceholderText("model, anim ...");

	pSelectTypes_ = new QToolButton();
	pSelectTypes_->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
	pSelectTypes_->setIcon(QIcon(":/style/img/down_arrow.png"));

	QGridLayout* pLayout = new QGridLayout();

	// name
	{
		pLayout->addWidget(pNameFilterEnabled_, 0, 0);
		pLayout->addWidget(pNameFilter_, 0, 1, 1, 2);
	}

	{
		pLayout->addWidget(pTypeFilterEnabled_, 1, 0);
		pLayout->addWidget(pTypesFilter_, 1, 1);
		pLayout->addWidget(pSelectTypes_, 1, 2);
	}


	connect(pSelectTypes_, SIGNAL(clicked()),
		this, SLOT(showTypeSeelction()));

	connect(pTypesFilter_, &QLineEdit::textEdited, this, &AssetDbFilterOptionsWidget::typeFilterTextEditied);
	connect(pTypesFilter_, &QLineEdit::textEdited, this, &AssetDbFilterOptionsWidget::nameFilterTextEditied);

	setLayout(pLayout);
}



void AssetDbFilterOptionsWidget::showTypeSeelction(void)
{
	QWidget* pSender = qobject_cast<QWidget*>(sender());
	BUG_ASSERT(pSender, return);

	// ya done fucked it.
	QListWidget* pList = new QListWidget();
	pList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	for (uint32_t i = 0; i < assetDb::AssetType::ENUM_COUNT; i++)
	{
		QListWidgetItem* pItem = new QListWidgetItem(typeNamesCache_[i], pList);
		pItem->setFlags(pItem->flags() & ~Qt::ItemIsUserCheckable);

		if (typesEnabled_[i]) {
			pItem->setCheckState(Qt::Checked);
		}
		else {
			pItem->setCheckState(Qt::Unchecked);
		}
	}

	connect(pList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(typeFilterChanged(QListWidgetItem*)));
	connect(pList, &QListWidget::itemClicked, this, &AssetDbFilterOptionsWidget::typeFilterClicked);

	QMenu menu;

	QWidgetAction* pWidgetAction = new QWidgetAction(&menu);
	pWidgetAction->setDefaultWidget(pList);

	menu.addAction(pWidgetAction);
	menu.exec(pSender->mapToGlobal(QPoint(0,0)));
}

void AssetDbFilterOptionsWidget::typeFilterChanged(QListWidgetItem* pItem)
{
	bool checked = pItem->checkState() == Qt::Checked;
	const int32_t index = pItem->listWidget()->row(pItem);
	BUG_ASSERT(index > 0 && index < assetDb::AssetType::ENUM_COUNT, return);


	typesEnabled_[index] = checked;

	QString text;

	// just remake the text?
	for (uint32_t i = 0; i < assetDb::AssetType::ENUM_COUNT; i++)
	{
		if (!typesEnabled_[i]) {
			continue;
		}

		text += typeNamesCache_[i] + ",";
	}

	if (!text.isEmpty()) {
		text = text.left(text.length() - 1);
	}

	pTypesFilter_->setText(text);
}

void AssetDbFilterOptionsWidget::typeFilterClicked(QListWidgetItem* pItem)
{
	if (pItem)
	{
		auto state = pItem->checkState();

		if (state == Qt::Checked) {
			pItem->setCheckState(Qt::Unchecked);
		}
		else {
			pItem->setCheckState(Qt::Checked);
		}
	}
}

void AssetDbFilterOptionsWidget::typeFilterTextEditied(const QString& text)
{
	// clear everything each time.
	// they may of passted completly diffrent filter types.
	core::zero_object(typesEnabled_);

	if (!text.isEmpty())
	{
		QStringList tokens = text.split(QChar(','));

		// Log(t*n)
		for (const auto& token : tokens)
		{
			// work out if it's a type.
			const QString tokenLow = token.toLower();

			for (int32_t i = 0; i < assetDb::AssetType::ENUM_COUNT; i++)
			{
				if (!typesEnabled_[i] && // early out string check.. worth it?
					typeNamesCache_[i] == tokenLow)
				{
					typesEnabled_[i] = true;
					break;
				}
			}
		}
	}

}

void AssetDbFilterOptionsWidget::nameFilterTextEditied(const QString& text)
{
	X_UNUSED(text);

}

// ------------------------------------------------------------------


AssetDbViewWidget::AssetDbViewWidget(AssetDB& db, QWidget *parent) :
    QWidget(parent),
    explorer_(AssetExplorer::instance()),
    model_(nullptr),
    view_(nullptr),
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

	connect(pSessionManager, SIGNAL(sessionLoaded()),
		this, SLOT(loadExpandData()));
	connect(pSessionManager, SIGNAL(aboutToSaveSession()),
		this, SLOT(saveExpandData()));


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

	AssetDbFilterOptionsWidget* pWSearchWidget = new AssetDbFilterOptionsWidget(this);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(view_);
    mainLayout->addWidget(pWSearchWidget);
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

void AssetDbViewWidget::expandAll(void)
{
	view_->expandAll();
}

void AssetDbViewWidget::expandBelow(void)
{
	if (view_->selectionModel()->currentIndex().isValid()) {
		view_->expandRecursive(view_->selectionModel()->currentIndex());
	}
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
    Node* pNode = model_->nodeForIndex(mainIndex);
    if (pNode->nodeType() != NodeType::FileNodeType) {
        return;
    }

	if (FileNode* pFileNode = qobject_cast<FileNode*>(pNode)) {
		EditorManager::openEditor(pFileNode->name(), pFileNode->assetType(), assman::Constants::ASSETPROP_EDITOR_ID);
	}
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


void AssetDbViewWidget::disableAutoExpand(void)
{
    autoExpand_ = false;
}


void AssetDbViewWidget::saveExpandData(void)
{
	QList<QVariant> data;
	recursiveSaveExpandData(view_->rootIndex(), &data);

	SessionManager::setValue(QLatin1String("AssetDbTree.ExpandData"), data);
}


void AssetDbViewWidget::loadExpandData(void)
{
	QList<QVariant> data = SessionManager::value(QLatin1String("AssetDbTree.ExpandData")).value<QList<QVariant>>();

	QSet<QString> set;

	for (const auto& v : data) {
		set.insert(v.toString());
	}

	recursiveLoadExpandData(view_->rootIndex(), set);
}


void AssetDbViewWidget::recursiveSaveExpandData(const QModelIndex &index, QList<QVariant> *data)
{
	Q_ASSERT(data);

	if (view_->isExpanded(index) || index == view_->rootIndex())
	{
		Node* pNode = model_->nodeForIndex(index);
		const QString list = QString(pNode->displayName());
		data->append(QVariant::fromValue(list));
		const int32_t count = model_->rowCount(index);
		for (int32_t i = 0; i < count; ++i) {
			recursiveSaveExpandData(index.child(i, 0), data);
		}
	}
}

void AssetDbViewWidget::recursiveLoadExpandData(const QModelIndex& index, QSet<QString> &data)
{
	Node *node = model_->nodeForIndex(index);
	const QString path = node->displayName();
	const QString displayName = node->displayName();
	auto it = data.find(displayName);

	if (it != data.end())
	{
		view_->expand(index);
		data.erase(it);
		const int32_t count = model_->rowCount(index);
		for (int32_t i = 0; i < count; ++i) {
			recursiveLoadExpandData(index.child(i, 0), data);
		}
	}
}


} // namespace AssetExplorer

X_NAMESPACE_END
