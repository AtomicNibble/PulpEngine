#include "stdafx.h"
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
	return nullptr;
}

X_NAMESPACE_END