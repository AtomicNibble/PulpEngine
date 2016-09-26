#include "stdafx.h"
#include "AssetPropertyEditorFactory.h"
#include "AssetPropertyEditor.h"

X_NAMESPACE_BEGIN(assman)


AssetPropertyEditorFactory::AssetPropertyEditorFactory(QObject *parent) :
	IEditorFactory(parent)
{
	setId(Constants::ASSETPROP_EDITOR_ID);
	setDisplayName(tr(Constants::C_ASSETPROP_EDITOR_DISPLAY_NAME));

}


IEditor* AssetPropertyEditorFactory::createEditor(void)
{
	AssetPropertyEditorWidget* editor = new AssetPropertyEditorWidget;
	return editor->editor();
}

X_NAMESPACE_END