#pragma once

#include <QObject>
#include "DirectionConstants.h"
#include "AssetEntryModel.h"


X_NAMESPACE_BEGIN(assman)

class IAssetEntry;
class IEditor;
class IEditorFactory;
class EditorView;
class SplitterOrView;
class BaseWindow;


class EditorManager : public QWidget
{
	Q_OBJECT

public:
	typedef QList<IEditorFactory*> EditorFactoryList;

	enum OpenEditorFlag {
		DoNotChangeCurrentEditor = 1,
		IgnoreNavigationHistory = 2,
		DoNotMakeVisible = 4,
		OpenInOtherSplit = 8
	};

	Q_DECLARE_FLAGS(OpenEditorFlags, OpenEditorFlag)

public:
	static EditorManager* instance(void);

	static QByteArray saveState(void);
	static bool restoreState(const QByteArray& state);

	static IAssetEntry* currentAssetEntry(void);
	static IEditor* currentEditor(void);
	static QList<IEditor*> visibleEditors(void);
	static QList<IEditor*> openEditorsList(void);
	static QList<EditorView*> visibleViews(void);

	static IEditor* openEditor(const QString& assetName, assetDb::AssetType::Enum type, const Id& editorId,
		OpenEditorFlags flags = 0, bool* pNewEditor = nullptr);



	static void activateEditor(IEditor* pEditor, OpenEditorFlags flags = 0);
	static void activateEditorForEntry(AssetEntryModel::Entry *entry, OpenEditorFlags flags = 0);
	static IEditor *activateEditorForAssetEntry(IAssetEntry* pAssetEntry, OpenEditorFlags flags = 0);
	static IEditor *activateEditorForAssetEntry(EditorView* pView, IAssetEntry* pAssetEntry, OpenEditorFlags flags = 0);

//	static DocumentModel *documentModel(void);
	static bool closeAssetEntrys(const QList<IAssetEntry*>& assetEntrys, bool askAboutModifiedEditors = true);
	static void closeEditor(AssetEntryModel::Entry *entry);
	static void closeOtherEditors(IAssetEntry* pAssetEntry);

	static bool saveEditor(IEditor* pEditor);

	static bool closeEditors(const QList<IEditor *>& editorsToClose, bool askAboutModifiedEditors = true);
	static void closeEditor(IEditor* pEditor, bool askAboutModifiedEditors = true);


	static qint64 maxTextFileSize(void);

	static void setWindowTitleAddition(const QString& addition);
	static QString windowTitleAddition(void);

	static void setWindowTitleVcsTopic(const QString& topic);
	static QString windowTitleVcsTopic(void);

	static void AddFactory(IEditorFactory* factory);

//	static void setReloadSetting(IDocument::ReloadSetting behavior);
//	static IDocument::ReloadSetting reloadSetting(void);

	static void addSaveAndCloseEditorActions(QMenu *contextMenu, IEditor* pEditor);
	static void addNativeDirActions(QMenu *contextMenu, IEditor* pEditor);
	static void addFloatActions(QMenu *contextMenu, IEditor* pEditor);


	static void setAutoSaveEnabled(bool enabled);
	static bool autoSaveEnabled(void);
	static void setAutoSaveInterval(int interval);
	static int autoSaveInterval(void);

	static void floatDockCheck(SplitterOrView* pSplitter, const QPoint& pos);

signals:
	void currentEditorChanged(IEditor* pEditor);
	void currentAssetEntryStateChanged(void);
	void editorCreated(IEditor* pEditor, const QString& assetName);
	void editorOpened(IEditor* pEditor);
	void editorAboutToClose(IEditor* pEditor);
	void editorsClosed(QList<IEditor *> editors);
	void findOnFileSystemRequest(const QString& path);


public slots:
	static bool closeAllEditors(bool askAboutModifiedEditors = true);

	static bool saveAssetEntry(IAssetEntry* pAssetEntry = nullptr);
	static bool saveAssetEntryAs(IAssetEntry* pAssetEntry = nullptr);

	void handleAssetEntryStateChange(void);

private slots:
	static void autoSave(void);
	static void updateWindowTitle(void);

	static void rootDestroyed(QObject *root);
	static void setCurrentEditorFromContextChange(void);

	static void handleContextChange(const QList<IContext *>& context);

	static void updateActions(void);

	// Context Menu
	static void saveAssetEntryFromContextMenu(void);

	static void closeEditorFromContextMenu(void);
	static void closeOtherEditorsFromContextMenu(void);

	static void copyFullPath(void);

	static void floatEditor(void);
	static void dockEditorMain(void);

	static void newHozTabGroup(void);
	static void newVerTabGroup(void);


	static void moveNewHozTabGroup(void);
	static void moveNewVerTabGroup(void);
	// Context Menu - end


public slots:
	static void split(Qt::Orientation orientation, IEditor* pEditor = nullptr);
	static void split(void);
	static void splitSideBySide(void);
	static void splitNewWindow(void);

	static void removeCurrentSplit(void);
	static void removeAllSplits(void);

	static void moveEditor(Qt::Orientation orientation, IEditor* pEditor, Direction side = Direction::Default);

private:
	explicit EditorManager(QWidget *parent);
	~EditorManager(void);

	static void init(void);

	static IEditor* createEditor(const Id& id = Id(), const QString& fileName = QString());
	static void addEditor(IEditor* pEditor);
	static void removeEditor(IEditor* pEditor);

	static IEditor* openEditor(EditorView* pView, const QString& fileName, assetDb::AssetType::Enum type,
		const Id& id = Id(), OpenEditorFlags flags = 0, bool *newEditor = nullptr);

	static IEditor* placeEditor(EditorView* pView, IEditor* pEditor);
	static IEditor* duplicateEditor(IEditor* pEditor);
	static IEditor* activateEditor(EditorView* pView, IEditor* pEditor, OpenEditorFlags flags = 0);
	static void activateEditorForEntry(EditorView* pView, AssetEntryModel::Entry *entry, OpenEditorFlags flags = 0);
	static void setCurrentView(EditorView* pView);
	static void setCurrentEditor(IEditor* pEditor, bool ignoreNavigationHistory = false);
	static EditorView* currentEditorView(void);
	static EditorView* viewForEditor(IEditor* pEditor);
	static SplitterOrView* findRoot(const EditorView* pView, int* rootIndex = nullptr);


	static void closeView(EditorView* pView);
	static void emptyView(EditorView* pView);

	static void RemoveSplitIfEmpty(EditorView* pView);
	static void splitNewWindow(IEditor* pEditor);
	static void splitNewWindow(EditorView* pView, IEditor* pEditor = nullptr);
	static IEditor* pickUnusedEditor(EditorView **foundView = nullptr);
	static void addAssetEntryToRecentFiles(IAssetEntry* pAssetEntry);
	static void updateAutoSave(void);
	static void setCloseSplitEnabled(SplitterOrView* splitterOrView, bool enable);
	static void setupSaveActions(IAssetEntry* pAssetEntry, QAction* saveAction, QAction* revertToSavedAction);

	static SplitterOrView* undockEditor(IEditor* editor, QPoint& pos);
	static void splitDragEndWindow(BaseWindow*, SplitterOrView*);
	static void splitDragEnd(SplitterOrView*);

	static void AddEditorsToView(EditorView* source, EditorView* target, QList<IEditor*>& editors);
	static void AddEditorsToView(SplitterOrView* splitter, EditorView* target);

	static void ShowDropOverlay(bool show, const QRect& rect);

	friend class AssetManager;
	friend class SplitterOrView;
	friend class EditorView;
	friend class CustomTabWidget;
	friend class CustomTabWidgetBar;
	friend class BaseWindow;

};



X_NAMESPACE_END