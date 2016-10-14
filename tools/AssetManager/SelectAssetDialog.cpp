#include "stdafx.h"
#include "SelectAssetDialog.h"


#include <../AssetDB/AssetDB.h>


X_NAMESPACE_BEGIN(assman)


SelectAssetDialog::SelectAssetDialog(QWidget *parent, assetDb::AssetDB& db, assetDb::AssetType::Enum type) :
	QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
	db_(db),
	type_(type)
{
	setWindowTitle("Select Asset");

	QVBoxLayout* pLayout = new QVBoxLayout();

	{
		pList_ = new QListWidget();
		pList_->setUniformItemSizes(true);

		pLayout->addWidget(pList_);
	}


	{
		QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		pButtonBox->button(QDialogButtonBox::Ok)->setDefault(true);
		connect(pButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
		connect(pButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
		pLayout->addWidget(pButtonBox);
	}

	setLayout(pLayout);


	pList_->setUpdatesEnabled(false);
	pList_->blockSignals(true);

	assetDb::AssetDB::AssetInfoArr assets(g_arena);
	db_.GetAssetList(type_, assets);

	for (const auto& a : assets)
	{
		QString str(a.name.c_str());
		pList_->addItem(str);
	}

	pList_->setUpdatesEnabled(true);
	pList_->blockSignals(false);	
}

SelectAssetDialog::~SelectAssetDialog()
{
}


QString SelectAssetDialog::getSelectedName(void) const
{
	return selectedAssetName_;
}

void SelectAssetDialog::accept(void)
{
	const int32_t row = pList_->currentRow();
	if (row < 0) {
		return;
	}

	QListWidgetItem* pItem = pList_->item(row);
	QString item = pItem->text();

	selectedAssetName_ = item;
	done(QDialog::Accepted);
}

void SelectAssetDialog::reject(void)
{
	done(QDialog::Rejected);
}


void SelectAssetDialog::done(int32_t val)
{
	QDialog::done(val);
}



X_NAMESPACE_END