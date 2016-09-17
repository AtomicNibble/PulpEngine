#include "stdafx.h"
#include "AssetPropertyEditor.h"

X_NAMESPACE_BEGIN(assman)


AssetProperties::AssetProperties(AssetPropertEditorWidget* widget) :
	IAssetEntry(widget)
{

}

AssetProperties::~AssetProperties()
{

}


bool AssetProperties::isModified(void) const
{
	return false;
}

bool AssetProperties::isSaveAsAllowed(void) const
{
	return false;
}

// ----------------------------------------------------------------


AssetPropertEditorWidget::AssetPropertEditorWidget(QWidget *parent) :
	QScrollArea(parent),
	pEditor_(nullptr)
{
	assetProps_ = QSharedPointer<AssetProperties>(new AssetProperties(this));

}

AssetPropertEditorWidget::AssetPropertEditorWidget(AssetProperties* pAssetEntry, QWidget *parent) :
	QScrollArea(parent),
	pEditor_(nullptr)
{
	assetProps_ = QSharedPointer<AssetProperties>(pAssetEntry);

}

AssetPropertEditorWidget::AssetPropertEditorWidget(AssetPropertEditorWidget* pOther) :
	pEditor_(nullptr)
{
	assetProps_ = pOther->assetProps_;
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

AssetProperties* AssetPropertEditorWidget::assetProperties(void) const
{
	return assetProps_.data();
}

AssetPropertEditor* AssetPropertEditorWidget::createEditor(void)
{
	return new AssetPropertEditor(this);
}

// ----------------------------------------------------------------


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
	return pEditorWidget_->assetProperties();
}

Id AssetPropertEditor::id(void) const
{
	return Id(Constants::ASSETPROP_EDITOR_ID);
}


X_NAMESPACE_END