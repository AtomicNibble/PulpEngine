#include "stdafx.h"
#include "AssetPropertyEditorFactory.h"
#include "AssetPropertyEditor.h"

X_NAMESPACE_BEGIN(assman)


AssetPropertyEditorFactory::AssetPropertyEditorFactory(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, QObject *parent) :
	IEditorFactory(parent),
	db_(db),
	pPropScriptMan_(pPropScriptMan)
{
	setId(Constants::ASSETPROP_EDITOR_ID);
	setDisplayName(tr(Constants::C_ASSETPROP_EDITOR_DISPLAY_NAME));

}


IEditor* AssetPropertyEditorFactory::createEditor(void)
{
	AssetPropertyEditorWidget* editor = new AssetPropertyEditorWidget(db_, pPropScriptMan_);
	return editor->editor();
}

X_NAMESPACE_END