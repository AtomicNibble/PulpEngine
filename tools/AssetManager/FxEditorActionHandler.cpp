#include "FxEditorActionHandler.h"

#include "ICommand.h"
#include "ActionManager.h"
#include "ActionContainer.h"
#include "EditorManager.h"

#include "FxEditor.h"

X_NAMESPACE_BEGIN(assman)


FxEditorActionHandler::FxEditorActionHandler(QObject *parent, Id contextId) :
	QObject(parent),
	contextId_(contextId)
{
	createActions();
	connect(EditorManager::instance(), SIGNAL(currentEditorChanged(IEditor*)),
		this, SLOT(updateCurrentEditor(IEditor*)));
}


QAction* FxEditorActionHandler::registerAction(const Id &id,
	const char* pSlot,
	const QString& title,
	const QKeySequence& keySequence,
	const char* pMenueGroup,
	ActionContainer* pContainer)
{
	QAction* result = new QAction(title, this);
	ICommand* pCommand = ActionManager::registerAction(result, id, Context(contextId_));

	if (!keySequence.isEmpty()) {
		pCommand->setDefaultKeySequence(keySequence);
	}

	if (pContainer && pMenueGroup) {
		pContainer->addAction(pCommand, pMenueGroup);
	}

	connect(result, SIGNAL(triggered(bool)), this, pSlot);
	return result;
}


void FxEditorActionHandler::createActions(void)
{

	pUndoAction_ = registerAction(Constants::EDIT_UNDO,
		SLOT(undoAction()), tr("Undo"));
	pRedoAction_ = registerAction(Constants::EDIT_REDO,
		SLOT(redoAction()), tr("Redo"));
	pCopyAction_ = registerAction(Constants::EDIT_CUT,
		SLOT(cutAction()), tr("Cut"));
	pCutAction_ = registerAction(Constants::EDIT_COPY,
		SLOT(copyAction()), tr("Copy"));
	pPasteAction_ = registerAction(Constants::EDIT_PASTE,
		SLOT(pasteAction()), tr("Paste"));
}

void FxEditorActionHandler::updateUndoRedo(void)
{
	BUG_ASSERT(currentEditorWidget_, return);

	pUndoAction_->setEnabled(currentEditorWidget_->isUndoAvailable());
	pRedoAction_->setEnabled(currentEditorWidget_->isRedoAvailable());
}

void FxEditorActionHandler::updateActions(void)
{
	BUG_ASSERT(currentEditorWidget_, return);

	updateUndoRedo();
}

void FxEditorActionHandler::updateUndoAction(void)
{
	BUG_ASSERT(currentEditorWidget_, return);

	updateUndoRedo();
}


void FxEditorActionHandler::updateCopyAction(bool e)
{
	BUG_ASSERT(currentEditorWidget_, return);

	const bool hasCopyableText = e;

	if (pCutAction_) {
		pCutAction_->setEnabled(hasCopyableText && !currentEditorWidget_->isReadOnly());
	}
	if (pCopyAction_) {
		pCopyAction_->setEnabled(hasCopyableText);
	}
}

void FxEditorActionHandler::modificationChanged(bool m)
{
	BUG_ASSERT(currentEditorWidget_, return);
	Q_UNUSED(m);

	updateUndoRedo();
}


void FxEditorActionHandler::updateCurrentEditor(IEditor *editor)
{
	if (currentEditorWidget_) {
		currentEditorWidget_->disconnect(this);
	}
	currentEditorWidget_ = nullptr;

	// don't need to do anything if the editor's context doesn't match
	// (our actions will be disabled because our context will not be active)
	if (!editor || !editor->context().contains(contextId_)) {
		return;
	}

	FxEditorWidget* pEditorWidget = qobject_cast<FxEditorWidget*>(editor->widget());

	BUG_ASSERT(pEditorWidget, return); // editor has our context id, so shouldn't happen

	currentEditorWidget_ = pEditorWidget;
	connect(currentEditorWidget_, SIGNAL(undoAvailable(bool)), this, SLOT(updateUndoAction()));
	connect(currentEditorWidget_, SIGNAL(readOnlyChanged()), this, SLOT(updateActions()));
	connect(currentEditorWidget_, SIGNAL(modificationChanged(bool)), this, SLOT(modificationChanged(bool)));
	connect(currentEditorWidget_, SIGNAL(copyAvailable(bool)), this, SLOT(updateCopyAction(bool)));

	updateActions();
}


// action handlers.

void FxEditorActionHandler::undoAction(void)
{
	if (!currentEditorWidget_) {
		return;
	}

	currentEditorWidget_->undo();
}

void FxEditorActionHandler::redoAction(void)
{
	if (!currentEditorWidget_) {
		return;
	}

	currentEditorWidget_->redo();
}

void FxEditorActionHandler::copyAction(void)
{
	if (!currentEditorWidget_) {
		return;
	}

	currentEditorWidget_->copy();
}

void FxEditorActionHandler::cutAction(void)
{
	if (!currentEditorWidget_) {
		return;
	}

	currentEditorWidget_->cut();
}

void FxEditorActionHandler::pasteAction(void)
{
	if (!currentEditorWidget_) {
		return;
	}

	currentEditorWidget_->paste();
}


X_NAMESPACE_END