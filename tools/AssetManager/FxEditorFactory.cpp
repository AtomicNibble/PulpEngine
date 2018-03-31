#include "stdafx.h"
#include "FxEditorFactory.h"
#include "FxEditor.h"
#include "FxEditorActionHandler.h"


X_NAMESPACE_BEGIN(assman)


FxEditorFactory::FxEditorFactory(assetDb::AssetDB& db, QObject *parent) :
	IEditorFactory(parent),
	db_(db)
{
	setId(Constants::FX_EDITOR_ID);
	setDisplayName(tr(Constants::C_FX_EDITOR_DISPLAY_NAME));

	pActionHandler_ = new FxEditorActionHandler(this, Constants::C_FX_EDITOR);
}

FxEditorFactory::~FxEditorFactory()
{
	if (pActionHandler_) {
		delete pActionHandler_;
	}
}

IEditor* FxEditorFactory::createEditor(void)
{
	FxEditorWidget* editor = new FxEditorWidget(db_);
	return editor->editor();
}

X_NAMESPACE_END