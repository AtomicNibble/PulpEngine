#include "AssetPropertyEditorActionHandler.h"

#include "ICommand.h"
#include "ActionManager.h"
#include "ActionContainer.h"
#include "EditorManager.h"

#include "AssetPropertyEditor.h"

X_NAMESPACE_BEGIN(editor)


AssetPropsEditorActionHandler::AssetPropsEditorActionHandler(QObject *parent, Id contextId, uint optionalActions) :
	QObject(parent),
	optionalActions_(optionalActions),
	contextId_(contextId)
{
	createActions();
	connect(EditorManager::instance(), SIGNAL(currentEditorChanged(IEditor*)),
		this, SLOT(updateCurrentEditor(IEditor*)));
}


QAction* AssetPropsEditorActionHandler::registerAction(const Id &id,
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


void AssetPropsEditorActionHandler::createActions(void)
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

	pCollapseAllAction_ = registerAction(Constants::ASSETPROP_COLLAPSE_ALL,
		SLOT(collapseAll()), "Collapse All");
	pExpandAllAction_ = registerAction(Constants::ASSETPROP_UNCOLLAPSE_ALL,
		SLOT(expandAll()), "Expand All");
}

void AssetPropsEditorActionHandler::updateUndoRedo(void)
{
	BUG_ASSERT(currentEditorWidget_, return);

	pUndoAction_->setEnabled(currentEditorWidget_->isUndoAvailable());
	pRedoAction_->setEnabled(currentEditorWidget_->isRedoAvailable());
}

void AssetPropsEditorActionHandler::updateActions(void)
{
	BUG_ASSERT(currentEditorWidget_, return);

	updateUndoRedo();
}

void AssetPropsEditorActionHandler::updateUndoAction(void)
{
	BUG_ASSERT(currentEditorWidget_, return);

	updateUndoRedo();
}


void AssetPropsEditorActionHandler::updateCopyAction(bool e)
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

void AssetPropsEditorActionHandler::modificationChanged(bool m)
{
	BUG_ASSERT(currentEditorWidget_, return);
	Q_UNUSED(m);

	updateUndoRedo();
}


void AssetPropsEditorActionHandler::updateCurrentEditor(IEditor *editor)
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

	AssetPropertyEditorWidget* pEditorWidget = qobject_cast<AssetPropertyEditorWidget*>(editor->widget());

	BUG_ASSERT(pEditorWidget, return); // editor has our context id, so shouldn't happen

	currentEditorWidget_ = pEditorWidget;
	connect(currentEditorWidget_, SIGNAL(undoAvailable(bool)), this, SLOT(updateUndoAction()));
	connect(currentEditorWidget_, SIGNAL(readOnlyChanged()), this, SLOT(updateActions()));
	connect(currentEditorWidget_, SIGNAL(modificationChanged(bool)), this, SLOT(modificationChanged(bool)));
	connect(currentEditorWidget_, SIGNAL(copyAvailable(bool)), this, SLOT(updateCopyAction(bool)));

	updateActions();
}


// action handlers.

void AssetPropsEditorActionHandler::undoAction(void)
{
	if (!currentEditorWidget_) {
		return;
	}

	currentEditorWidget_->undo();
}

void AssetPropsEditorActionHandler::redoAction(void)
{
	if (!currentEditorWidget_) {
		return;
	}

	currentEditorWidget_->redo();
}

void AssetPropsEditorActionHandler::copyAction(void)
{
	if (!currentEditorWidget_) {
		return;
	}

	currentEditorWidget_->copy();
}

void AssetPropsEditorActionHandler::cutAction(void)
{
	if (!currentEditorWidget_) {
		return;
	}

	currentEditorWidget_->cut();
}

void AssetPropsEditorActionHandler::pasteAction(void)
{
	if (!currentEditorWidget_) {
		return;
	}

	currentEditorWidget_->paste();
}


void AssetPropsEditorActionHandler::collapseAll(void)
{
	if (!currentEditorWidget_) {
		return;
	}

	currentEditorWidget_->collapseAll();
}

void AssetPropsEditorActionHandler::expandAll(void)
{
	if (!currentEditorWidget_) {
		return;
	}

	currentEditorWidget_->expandAll();
}


X_NAMESPACE_END