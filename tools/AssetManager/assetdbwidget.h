#ifndef ASSETDBWIDGET_H
#define ASSETDBWIDGET_H


#include <QWidget>
#include <QTreeView>

class QLineEdit;

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(assman)

namespace AssetExplorer
{

	class AssetDbTreeView : public QTreeView
	{
		Q_OBJECT
	public:
		explicit AssetDbTreeView(QWidget *parent = 0);

		void expandRecursive(const QModelIndex &index);

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

	class AssetDbFilterOptionsWidget : public QWidget
	{
		Q_OBJECT

	public:
		explicit AssetDbFilterOptionsWidget(QWidget* parent = nullptr);

	private slots:
		void showTypeSeelction(void);
		void typeFilterChanged(QListWidgetItem* pItem);
		void typeFilterClicked(QListWidgetItem* pItem);
		void typeFilterTextEditied(const QString& text);
		void nameFilterTextEditied(const QString& text);

	private:
		QCheckBox* pNameFilterEnabled_;
		QCheckBox* pTypeFilterEnabled_;

		QLineEdit* pNameFilter_;
		QLineEdit* pTypesFilter_;

		QToolButton* pSelectTypes_;
	
		QString typeNamesCache_[assetDb::AssetType::ENUM_COUNT];
		bool typesEnabled_[assetDb::AssetType::ENUM_COUNT];
	};

	class AssetDbViewWidget : public QWidget
	{
		Q_OBJECT

	public:
		typedef assetDb::AssetDB AssetDB;

	public:
		explicit AssetDbViewWidget(AssetDB& db, QWidget* parent = 0);

	signals:

	public slots :
		void collapseAll(void);
		void expandAll(void);
		void expandBelow(void);
		void editCurrentItem(void);

	private slots:
		void initView(void);

		void showContextMenu(const QPoint &pos);

		void setCurrentItem(Node* pNode, Project* pProject);
		void openItem(const QModelIndex &mainIndex);
		void handleProjectAdded(Project* pProject);
		void startupProjectChanged(Project* pProject);

		void foldersAboutToBeRemoved(FolderNode*, const QList<FolderNode*> &);
		void filesAboutToBeRemoved(FolderNode*, const QList<FileNode*> &);

		void disableAutoExpand(void);


		void saveExpandData(void);
		void loadExpandData(void);

	private:
		void recursiveSaveExpandData(const QModelIndex &index, QList<QVariant> *data);
		void recursiveLoadExpandData(const QModelIndex &index, QSet<QString>& data);

	private:
		IContext* pContext_;
		AssetExplorer* explorer_;
		AssetDBModel* model_;
		AssetDbTreeView*  view_;

		bool autoExpand_;
	};


} // namespace AssetExplorer

X_NAMESPACE_END


#endif // ASSETDBWIDGET_H
