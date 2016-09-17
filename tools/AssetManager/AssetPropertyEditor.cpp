#include "stdafx.h"
#include "AssetPropertyEditor.h"

X_NAMESPACE_BEGIN(assman)


AssetProperties::AssetProperties(AssetPropertyEditorWidget* widget) :
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


AssetPropertyEditorWidget::AssetPropertyEditorWidget(QWidget *parent) :
	QScrollArea(parent),
	pEditor_(nullptr)
{
	assetProps_ = QSharedPointer<AssetProperties>(new AssetProperties(this));

}

AssetPropertyEditorWidget::AssetPropertyEditorWidget(AssetProperties* pAssetEntry, QWidget *parent) :
	QScrollArea(parent),
	pEditor_(nullptr)
{
	assetProps_ = QSharedPointer<AssetProperties>(pAssetEntry);

}

AssetPropertyEditorWidget::AssetPropertyEditorWidget(AssetPropertyEditorWidget* pOther) :
	pEditor_(nullptr)
{
	assetProps_ = pOther->assetProps_;
}

AssetPropertyEditorWidget::~AssetPropertyEditorWidget()
{

}



bool AssetPropertyEditorWidget::open(QString* pErrorString, const QString& fileName)
{
	X_UNUSED(pErrorString);
	X_UNUSED(fileName);

	AssetProperties* pProbs = assetProps_.data();

	pProbs->setDisplayName(fileName);
	return true;
}

AssetPropertyEditor* AssetPropertyEditorWidget::editor(void)
{
	if (!pEditor_) {
		pEditor_ = createEditor();
	}

	return pEditor_;
}

AssetProperties* AssetPropertyEditorWidget::assetProperties(void) const
{
	return assetProps_.data();
}

AssetPropertyEditor* AssetPropertyEditorWidget::createEditor(void)
{
	return new AssetPropertyEditor(this);
}

// ----------------------------------------------------------------


AssetPropertyEditor::AssetPropertyEditor(AssetPropertyEditorWidget* editor) :
	pEditorWidget_(editor)
{
	setWidget(pEditorWidget_);


}

AssetPropertyEditor::~AssetPropertyEditor()
{
	delete pEditorWidget_;
}


bool AssetPropertyEditor::open(QString* pErrorString, const QString& fileName)
{
	return pEditorWidget_->open(pErrorString, fileName);
}

IAssetEntry* AssetPropertyEditor::assetEntry(void)
{
	return pEditorWidget_->assetProperties();
}

Id AssetPropertyEditor::id(void) const
{
	return Id(Constants::ASSETPROP_EDITOR_ID);
}


X_NAMESPACE_END