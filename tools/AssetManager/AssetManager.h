#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <QMainWindow>

class QGridLayout;

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(assman)

class ActionManager;
class EditorManager;
class VersionDialog;

namespace AssetExplorer {
    class AssetDbViewWidget;
    class AssetExplorer;
}

class AssetManager : public QMainWindow
{
    Q_OBJECT

public:
    explicit AssetManager(QWidget* pParent = nullptr);
    ~AssetManager();

	IContext* currentContextObject(void) const;
	IContext* contextObject(QWidget* pWidget);
	void addContextObject(IContext* pContex);
	void removeContextObject(IContext* pContex);

private:
	void createActions(void);
	void createMenus(void);
	void createStatusBar(void);
	void createDockWindows(void);

private:
	void updateContextObject(const QList<IContext*>& context);
	void updateContext(void);

	template<class T>
	T* AddDockItem(const char* pName, Qt::DockWidgetAreas areas, Qt::DockWidgetArea initial);
	template<class T>
	void AddDockItem(const char* pName, T* pWidget, Qt::DockWidgetAreas areas, Qt::DockWidgetArea start);

public slots:
	void raiseWindow(void);


private slots:
	void updateFocusWidget(QWidget* pOld, QWidget* pNow);


	// File Slots
	void about(void);
	void destroyAboutDialog(void);

protected:
	virtual void changeEvent(QEvent* e) X_OVERRIDE;
	virtual void closeEvent(QCloseEvent *event) X_OVERRIDE;


private:

	// File
	QAction* pSaveAllAct_;
	QAction* pQuitAct_;
	// Help
	QAction* pAboutAct_;
	QAction* pAboutQtAct_;


private:
	VersionDialog* pVersionDialog_;

private:
    QGridLayout* pLayout_;

	ICore*  pCoreImpl_;
	Context additionalContexts_;

	ActionManager* pActionManager_;
	EditorManager* pEditorManager_;

	// context baby, do you speak it!
	QList<IContext*>           activeContext_;
	QMap<QWidget*, IContext*> contextWidgets_;

	assetDb::AssetDB* pDb_;
	AssetExplorer::AssetDbViewWidget* pAssetViewWidget_;
    AssetExplorer::AssetExplorer* pAssetDbexplorer_;
};

X_NAMESPACE_END

#endif // ASSETMANAGER_H
