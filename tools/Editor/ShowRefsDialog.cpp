#include "stdafx.h"
#include "ShowRefsDialog.h"


#include <../AssetDB/AssetDB.h>


X_NAMESPACE_BEGIN(editor)


AssetRefsDialog::AssetRefsDialog(QWidget *parent, assetDb::AssetDB& db, assetDb::AssetType::Enum type, const QString& name) :
	QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
	db_(db),
	type_(type),
	name_(name.toLatin1()),
	infoLoaded_(false)
{
	setWindowTitle(QString("Asset Refs - %1").arg(name));


	pRefList_ = new QListWidget(this);

	pFormLayout_ = new QFormLayout();
	pFormLayout_->addRow("Refrences", pRefList_);

	{
		QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
		pButtonBox->button(QDialogButtonBox::Ok)->setDefault(true);
		connect(pButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
		connect(pButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
		pFormLayout_->addWidget(pButtonBox);
	}

	setLayout(pFormLayout_);
}


AssetRefsDialog::~AssetRefsDialog()
{
}


bool AssetRefsDialog::loadInfo(void)
{
	// get asset id.
	int32_t assetId;
	if (!db_.AssetExsists(type_, name_, &assetId)) {
		X_ERROR("AssetRefs", "Can't delete asset it does not exsist.");
		return false;
	}

	assetDb::AssetDB::AssetIdArr refs(g_arena);
	if (!db_.GetAssetRefs(assetId, refs)) {
		X_ERROR("AssetRefs", "Failed to get asset refs");
		return false;
	}

	if (refs.isNotEmpty())
	{
		assetDb::AssetDB::AssetInfo info;
		for (const auto& ref : refs)
		{
			if (!db_.GetAssetInfoForAsset(ref, info)) {
				X_ERROR("AssetRefs", "Failed to get asset refs info");
				return false;
			}

			QString name = QString::fromLocal8Bit(info.name.c_str(), static_cast<int32_t>(info.name.length()));
			pRefList_->addItem(name);
		}
	}

	infoLoaded_ = true;
	return true;
}


void AssetRefsDialog::accept(void)
{
	if (!infoLoaded_) {
		QMessageBox::critical(this, "Asset Refs", "Source code error, asset info not loaded.", QMessageBox::Ok);
	}

	// we don't do anything.
	done(QDialog::Accepted);
}

void AssetRefsDialog::reject(void)
{
	done(QDialog::Rejected);
}


void AssetRefsDialog::done(int32_t val)
{
	QDialog::done(val);
}



X_NAMESPACE_END