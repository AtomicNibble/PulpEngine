#include "stdafx.h"
#include "SelectAssetDialog.h"


#include <../AssetDB/AssetDB.h>


X_NAMESPACE_BEGIN(editor)


SelectAssetDialog::SelectAssetDialog(QWidget *parent, assetDb::AssetDB& db, assetDb::AssetType::Enum type) :
	QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
	db_(db),
	type_(type)
{
	setWindowTitle(QString("Select Asset - %1").arg(assetDb::AssetType::ToString(type)));

	QVBoxLayout* pLayout = new QVBoxLayout();

	pSortFilter_ = new  QSortFilterProxyModel(this);
	pSortFilter_->setSourceModel(&items_);

	{
		pSearch_ = new QLineEdit(this);
		pSearch_->setPlaceholderText(tr("Search..."));

		connect(pSearch_, SIGNAL(textChanged(QString)), pSortFilter_, SLOT(setFilterFixedString(QString)));

		pLayout->addWidget(pSearch_);
	}

	{
		pList_ = new QListView(this);
		pList_->setUniformItemSizes(true);
		pList_->setModel(pSortFilter_);
		pList_->setEditTriggers(QAbstractItemView::NoEditTriggers);

		connect(pList_, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(processSelected(const QModelIndex &)));


		pLayout->addWidget(pList_);
	}


	{
		QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
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

	QStringList items;
	items.reserve(static_cast<int32_t>(assets.size()));

	for (const auto& a : assets)
	{
		QString str(a.name.c_str());
		items.append(str);
	}

	items_.setStringList(items);

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

void SelectAssetDialog::processSelected(const QModelIndex& index)
{
	if(!index.isValid()) {
		return;
	}

	const auto data = pSortFilter_->data(index, 0);
	if (!data.isValid()) {
		return;
	}

	if (data.type() != QVariant::String) {
		return;
	}

	QString name = data.toString();

	selectedAssetName_ = name;
	done(QDialog::Accepted);
}

void SelectAssetDialog::accept(void)
{
	const auto row = pList_->currentIndex();

	processSelected(row);
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