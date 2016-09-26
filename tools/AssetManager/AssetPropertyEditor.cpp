#include "stdafx.h"
#include "AssetPropertyEditor.h"

#include "AssetScript.h"
#include "AssetScriptTypes.h"


#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(assman)


AssetProperties::AssetProperties(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, AssetPropertyEditorWidget* widget) :
	IAssetEntry(widget),
	db_(db),
	pPropScriptMan_(pPropScriptMan),
	pWidget_(widget)
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


AssetPropertyEditorWidget* AssetProperties::getEditor(void)
{
	return pWidget_;
}

// ----------------------------------------------------------------



AssetPropertyEditorWidget::AssetPropertyEditorWidget(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, QWidget *parent) :
	QScrollArea(parent),
	db_(db),
	pPropScriptMan_(pPropScriptMan),
	pEditor_(nullptr)
{
	assetProps_ = QSharedPointer<AssetProperties>(new AssetProperties(db, pPropScriptMan, this));
}

AssetPropertyEditorWidget::AssetPropertyEditorWidget(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, AssetProperties* pAssetEntry, QWidget *parent) :
	QScrollArea(parent),
	db_(db),
	pPropScriptMan_(pPropScriptMan),
	pEditor_(nullptr)
{
	assetProps_ = QSharedPointer<AssetProperties>(pAssetEntry);

}

AssetPropertyEditorWidget::AssetPropertyEditorWidget(AssetPropertyEditorWidget* pOther) :
	pPropScriptMan_(pOther->pPropScriptMan_),
	db_(pOther->db_),
	pEditor_(nullptr)
{
	assetProps_ = pOther->assetProps_;
}

AssetPropertyEditorWidget::~AssetPropertyEditorWidget()
{

}



bool AssetPropertyEditorWidget::open(QString* pErrorString, const QString& fileName, assetDb::AssetType::Enum type)
{
	X_UNUSED(pErrorString);
	X_UNUSED(fileName);
	X_UNUSED(type);

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


bool AssetPropertyEditor::open(QString* pErrorString, const QString& fileName, assetDb::AssetType::Enum type)
{
	return pEditorWidget_->open(pErrorString, fileName, type);
}

IAssetEntry* AssetPropertyEditor::assetEntry(void)
{
	return pEditorWidget_->assetProperties();
}

Id AssetPropertyEditor::id(void) const
{
	return Id(Constants::ASSETPROP_EDITOR_ID);
}

void AssetPropertyEditor::modificationChanged(bool modified)
{
	QString title = assetEntry()->displayName();

	if (modified) {
		title += "*";
	}

	emit titleChanged(title);
}

X_NAMESPACE_END