#include "AssetAssetRefWidget.h"
#include "SelectAssetDialog.h"

#include "IAssetEntry.h"

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(assman)


AssetAssetRefWidget::AssetAssetRefWidget(QWidget *parent, assetDb::AssetDB& db, IAssetEntry* pAssEntry, 
	const std::string& typeStr, const std::string& value) :
	QWidget(parent),
	db_(db),
	pAssEntry_(pAssEntry),
	type_(assetDb::AssetType::MODEL)
{
	// work out the type.
	core::StackString<96, char> typeStrLower(typeStr.c_str(), typeStr.c_str() + typeStr.length());
	core::StackString<96, char> temp;

	typeStrLower.toLower();
	int32_t i;
	for (i = 0; i < assetDb::AssetType::ENUM_COUNT; i++)
	{
		const char* pName = assetDb::AssetType::ToString(i);

		temp.set(pName);
		temp.toLower();

		if (temp == typeStrLower)
		{
			type_ = static_cast<assetDb::AssetType::Enum>(i);
			break;
		}
	}

	// this is for selecting a asset of type 'type'
	// so we want a edit to show the name and a button for selecting.
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);

	pLineEdit_ = new QLineEdit(this);
	pLineEdit_->setReadOnly(true);

	// browse button
	QToolButton* pBrowse = new QToolButton();
	pBrowse->setText("...");
	connect(pBrowse, SIGNAL(clicked()), this, SLOT(browseClicked()));

	pLayout->addWidget(pLineEdit_);
	pLayout->addWidget(pBrowse);

	setValue(value);

	setLayout(pLayout);

	// found a vlaid type?
	if (i == assetDb::AssetType::ENUM_COUNT) {
		X_ERROR("AssetRef", "Invalid asset type of \"%s\"", typeStr.c_str());

		pBrowse->setEnabled(false);
	}
}

AssetAssetRefWidget::~AssetAssetRefWidget()
{

}

void AssetAssetRefWidget::setValue(const std::string& value)
{
	pLineEdit_->blockSignals(true);
	pLineEdit_->setText(QString::fromStdString(value));
	pLineEdit_->blockSignals(false);
}

void AssetAssetRefWidget::browseClicked(void)
{
	// shieet.
	// you want to select a asset of type 'type'
	// this means we need to get a list of assets and show them.
	SelectAssetDialog dig(ICore::mainWindow(), db_, type_);

	if (dig.exec() == QDialog::Accepted)
	{
		const QString oldName = pLineEdit_->text();
		const QString assName = dig.getSelectedName();


		int32_t assetId;
		if (!db_.AssetExsists(pAssEntry_->type(), pAssEntry_->nameNarrow(), &assetId)) {
			X_ERROR("AssetRef", "Failed to get source asset id");
			return;
		}

		if (!removeRef(assetId, oldName)) {
			return;
		}

		// should we also add a ref to this asset?
		// fucking yes you slut.
		if (!addRef(assetId, assName)) {
			return;
		}

		pLineEdit_->setText(assName);

		emit valueChanged(assName.toStdString());
	}
}


bool AssetAssetRefWidget::removeRef(int32_t assetId, const QString& assName)
{
	if (assName.isEmpty()) {
		return true;
	}

	const std::string str = assName.toStdString();

	int32_t targetAssetId;
	if (!db_.AssetExsists(type_, core::string(str.data(), str.data() + str.length()), &targetAssetId)) {
		X_ERROR("AssetRef", "Failed to get ref asset's id for removal");
		return false;
	}

	auto res = db_.RemoveAssertRef(assetId, targetAssetId);
	if (res != assetDb::AssetDB::Result::OK) {
		X_ERROR("AssetRef", "Failed to remove exsisting asset ref");
		return false;
	}

	return true;
}

bool AssetAssetRefWidget::addRef(int32_t assetId, const QString& assName)
{
	const std::string str = assName.toStdString();

	int32_t targetAssetId;
	if (!db_.AssetExsists(type_, core::string(str.data(), str.data() + str.length()), &targetAssetId)) {
		X_ERROR("AssetRef", "Failed to get ref asset's id");
		return false;
	}

	auto res = db_.AddAssertRef(assetId, targetAssetId);
	if(res != assetDb::AssetDB::Result::OK) {
		X_ERROR("AssetRef", "Failed to add asset ref");
		return false;
	}

	return true;
}


X_NAMESPACE_END