#pragma once

#include <QObject>

class QAction;


X_NAMESPACE_BEGIN(assman)

class ICore;
class IEditor;
class ActionContainer;

class FxEditorWidget;

class FxEditorActionHandler : public QObject
{
	Q_OBJECT

public:
	explicit FxEditorActionHandler(QObject *parent, Id contextId);
	~FxEditorActionHandler() = default;

private:
	QAction* registerAction(const Id &id,
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

private:
	QAction* pUndoAction_;
	QAction* pRedoAction_;
	QAction* pCopyAction_;
	QAction* pCutAction_;
	QAction* pPasteAction_;

	QPointer<FxEditorWidget> currentEditorWidget_;
	Id contextId_;
};

X_NAMESPACE_END