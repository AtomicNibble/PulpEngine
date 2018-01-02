#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <QMainWindow>
#include "BaseWindow.h"

// Logging
#include "Logging\Logger.h"
#include "Logging\FilterPolicies\LoggerNoFilterPolicy.h"
#include "Logging\FormatPolicies\LoggerSimpleFormatPolicy.h"
#include "LoggerOutputWindowWritePolicy.h"

class QGridLayout;

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(editor)

class ConverterHost;
class ActionManager;
class AssetEntryManager;
class EditorManager;
class VersionDialog;
class AssetPropsScriptManager;
class MyStatusBar;


class OutputWindow;

typedef core::Logger<
	core::LoggerNoFilterPolicy,
	core::LoggerSimpleFormatPolicyStripColors,
	editor::LoggerOutputWindowWritePolicy>
	OutputWindowWrtiePolicy;


namespace AssetExplorer {
    class AssetDbViewWidget;
    class AssetExplorer;
}

class Editor : public BaseWindow
{
	Q_OBJECT

	static const char* SETTINGS_GROUP;
	static const char* WINDOW_GEOMETRY_KEY;
	static const char* WINDOW_STATE_KEY;

public:
    explicit Editor(QWidget* pParent = nullptr);
    ~Editor();

	IContext* currentContextObject(void) const;
	IContext* contextObject(QWidget* pWidget);
	void addContextObject(IContext* pContex);
	void removeContextObject(IContext* pContex);

	QSettings* settings(QSettings::Scope scope = QSettings::Scope::UserScope) const;

	MyStatusBar* statusBar(void);


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


private slots:
	void updateFocusWidget(QWidget* pOld, QWidget* pNow);

	// file Closes
	void newFile(void);
	void newMod(void);
	void save(void);
	void saveAll(void);

	void aboutToShowRecentFiles(void);
	void openRecentFile(void);

	// View Slots
	void aboutToShowViewMenu(void);

	// Window Slots
	void resetLayout(void);
	void reloadStyle(void);
	void aboutToShowWindowMenu(void);
	void windowListSetActiveEditor(void);

	// File Slots
	void about(void);
	void destroyAboutDialog(void);

	// misc
	void fileChanged(const QString& path);

protected:
	virtual void closeEvent(QCloseEvent *event) X_OVERRIDE;
	bool event(QEvent *e) X_OVERRIDE;

private:
	void readSettings(void);
	void saveSettings(void);
	void saveWindowState(void);
	void restoreWindowState(void);
	void reloadStyle(const QString& path);

private:

	// File
	QAction* pNewFileAct_;
	QAction* pNewModAct_;
	QAction* pSaveAct_;
	QAction* pSaveAllAct_;
	QAction* pQuitAct_;

	// Edit
	QAction* pUndoAct_;
	QAction* pRedoAct_;
	QAction* pCutAct_;
	QAction* pCopyAct_;
	QAction* pPasteAct_;

	// View
	QAction* pViewAssetDbExpoAct_;

	// Window
	QAction* pWindowResetLayoutAct_;
	QAction* pReloadStyleAct_;

	// Help
	QAction* pAboutAct_;
	QAction* pAboutQtAct_;

private:
	QSettings* pSettings_;

private:
	VersionDialog* pVersionDialog_;

private:
    QGridLayout* pLayout_;
	QMainWindow* pDockArea_;
	MyStatusBar* pStatusBar_;
	OutputWindow* pOutputWindow_;
	OutputWindowWrtiePolicy* pLoggerPolicy_;

private:
	QFileSystemWatcher* pWatcher_;

	ICore*  pCoreImpl_;
	Context additionalContexts_;

	ActionManager* pActionManager_;
	AssetEntryManager* pAssetEntryManager_;
	EditorManager* pEditorManager_;
	AssetPropsScriptManager* pAssetScripts_;

	// context baby, do you speak it!
	QList<IContext*>           activeContext_;
	QMap<QWidget*, IContext*> contextWidgets_;

	assetDb::AssetDB* pDb_;
	ConverterHost* pConHost_;
	AssetExplorer::AssetDbViewWidget* pAssetViewWidget_;
    AssetExplorer::AssetExplorer* pAssetDbexplorer_;
};

X_NAMESPACE_END

#endif // ASSETMANAGER_H
