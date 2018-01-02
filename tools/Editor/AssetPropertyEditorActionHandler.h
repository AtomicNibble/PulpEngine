#pragma once

#include <QObject>

class QAction;


X_NAMESPACE_BEGIN(assman)

class ICore;
class IEditor;
class ActionContainer;

class AssetPropertyEditorWidget;

class AssetPropsEditorActionHandler : public QObject
{
	Q_OBJECT

public:
	enum OptionalActionsMask {
		None = 0,
		CollapseAll = 1,
		UnCollapseAll = 2,
	};

public:
	explicit AssetPropsEditorActionHandler(QObject *parent, Id contextId, uint optionalActions = None);
	~AssetPropsEditorActionHandler() = default;


private:
	QAction *registerAction(const Id &id,
		const char *slot,
		const QString &title = QString(),
		const QKeySequence &keySequence = QKeySequence(),
		const char *menueGroup = 0,
		ActionContainer *container = 0);

private:
	void createActions(void);
	void updateUndoRedo(void);

private slots:
	void updateActions(void);
	void updateUndoAction(void);
	void updateCopyAction(bool);
	void modificationChanged(bool);

	void updateCurrentEditor(IEditor* pEditor);

	void undoAction(void);
	void redoAction(void);
	void copyAction(void);
	void cutAction(void);
	void pasteAction(void);

	void collapseAll(void);
	void expandAll(void);


private:
	QAction* pUndoAction_;
	QAction* pRedoAction_;
	QAction* pCopyAction_;
	QAction* pCutAction_;
	QAction* pPasteAction_;

	QAction* pCollapseAllAction_;
	QAction* pExpandAllAction_;


	uint32_t optionalActions_;
	QPointer<AssetPropertyEditorWidget> currentEditorWidget_;
	Id contextId_;
};

X_NAMESPACE_END