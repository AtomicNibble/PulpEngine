#include "AssetAssetRefWidget.h"

#include "SelectAssetDialog.h"

X_NAMESPACE_BEGIN(assman)


AssetAssetRefWidget::AssetAssetRefWidget(QWidget *parent, assetDb::AssetDB& db, assetDb::AssetType::Enum type, const std::string& value) :
	QWidget(parent),
	db_(db),
	type_(type)
{
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