#include "stdafx.h"
#include "AssetPropertyEditor.h"

X_NAMESPACE_BEGIN(assman)




AssetPropertEditorWidget::AssetPropertEditorWidget(QWidget *parent) :
	QScrollArea(parent),
	pEditor_(nullptr)
{

}

AssetPropertEditorWidget::~AssetPropertEditorWidget()
{

}



bool AssetPropertEditorWidget::open(QString* pErrorString, const QString& fileName)
{
	X_UNUSED(pErrorString);
	X_UNUSED(fileName);
	return true;
}

AssetPropertEditor* AssetPropertEditorWidget::editor(void)
{
	if (!pEditor_) {
		pEditor_ = createEditor();
	}

	return pEditor_;
}

AssetPropertEditor* AssetPropertEditorWidget::createEditor(void)
{
	return new AssetPropertEditor(this);
}

// ------------------------------------------


AssetPropertEditor::AssetPropertEditor(AssetPropertEditorWidget* editor) :
	pEditorWidget_(editor)
{
	setWidget(pEditorWidget_);


}

AssetPropertEditor::~AssetPropertEditor()
{
	delete pEditorWidget_;
}


bool AssetPropertEditor::open(QString* pErrorString, const QString& fileName)
{
	return pEditorWidget_->open(pErrorString, fileName);
}

IAssetEntry* AssetPropertEditor::assetEntry(void)
{
	return nullptr;
}

Id AssetPropertEditor::id(void) const
{
	return Id(Constants::ASSETPROP_EDITOR_ID);
}


X_NAMESPACE_END