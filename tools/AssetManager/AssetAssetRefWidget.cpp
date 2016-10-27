#include "AssetAssetRefWidget.h"

#include "SelectAssetDialog.h"

X_NAMESPACE_BEGIN(assman)


AssetAssetRefWidget::AssetAssetRefWidget(QWidget *parent, assetDb::AssetDB& db, const std::string& typeStr, const std::string& value) :
	QWidget(parent),
	db_(db),
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
		QString assName = dig.getSelectedName();

		pLineEdit_->setText(assName);

		// should we also add a ref to this asset?
		// fucking yes you slut.

		emit valueChanged(assName.toStdString());
	}
}


X_NAMESPACE_END