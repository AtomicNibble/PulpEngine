#include "stdafx.h"
#include "AddAssetDialog.h"


#include <../AssetDB/AssetDB.h>


X_NAMESPACE_BEGIN(assman)


AddAssetDialog::AddAssetDialog(QWidget *parent, assetDb::AssetDB& db) :
	QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
	db_(db)
{
	// hey, you looking at my code? you loco!
	// We want:
	//		name
	//		type
	//		mod ?
	setWindowTitle("Add Asset");

	QFormLayout* pFormLayout = new QFormLayout();

	{
		pAssetName_ = new QLineEdit();

		QRegularExpression re(QString("[a-z0-9_\\%1]*").arg(QChar(assetDb::ASSET_NAME_SLASH)));

		pAssetName_->setMaxLength(assetDb::ASSET_NAME_MAX_LENGTH);
		pAssetName_->setValidator(new QRegularExpressionValidator(re));

		pFormLayout->addRow("Name", pAssetName_);
	}

	{
		QStringList items;

		// shiieeeet, auto populate it from the type enum.
		for (uint32_t i = 0; i < assetDb::AssetType::ENUM_COUNT; i++)
		{
			const char* pName = assetDb::AssetType::ToString(i);
			QString name(pName);

			items.append(name.toLower());
		}

		pAssetType_ = new QComboBox();
		pAssetType_->addItems(items);

		pFormLayout->addRow("Type", pAssetType_);
	}

	{
		pMod_ = new QComboBox();

		// const int32_t curMod = db_.GetModId();
		assetDb::AssetDB::ModsArr mods(g_arena);

		if (!db_.GetModsList(mods))
		{
			pMod_->setDisabled(true);
			X_ERROR("AssetExplor", "Failed to iterate mods");
		}
		else
		{
			for (const auto& mod : mods)
			{
				QString name = QString::fromLocal8Bit(mod.name.c_str(), static_cast<int32_t>(mod.name.length()));
		
				pMod_->addItem(name, QVariant(mod.modId));
			}
		}

		pFormLayout->addRow("Mod", pMod_);
	}


	{
		QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		pButtonBox->button(QDialogButtonBox::Ok)->setDefault(true);
		connect(pButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
		connect(pButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
		pFormLayout->addWidget(pButtonBox);
	}


	setLayout(pFormLayout);
}

AddAssetDialog::~AddAssetDialog()
{
}

void AddAssetDialog::accept(void)
{
	assetDb::AssetType::Enum type;
	core::string assName;
	int32_t modId = assetDb::AssetDB::INVALID_MOD_ID;

	// name
	{
		// my nuggger.
		QString name = pAssetName_->text();

		if (name.isEmpty()) {
			QMessageBox::warning(this, "Invalid asset name", "Asset name is empty", QMessageBox::Ok);
			return;
		}

		if (name.length() < assetDb::ASSET_NAME_MIN_LENGTH) {
			QMessageBox::warning(this, "Invalid asset name", "Asset name is too short", QMessageBox::Ok);
			return;
		}


		const auto latinStr = name.toLatin1();

		assName = latinStr;
	}

	// type
	{
		const int32_t index = pAssetType_->currentIndex();

		if (index < 0 || index >= assetDb::AssetType::ENUM_COUNT) {
			QMessageBox::warning(this, "Invalid asset type", "Error getting asset type. (slap a dev)", QMessageBox::Ok);
			return;
		}

		type = static_cast<assetDb::AssetType::Enum>(index);
	}

	// mod
	{
		const int32_t index = pAssetType_->currentIndex();
		auto variant = pAssetType_->itemData(index);

		modId = variant.toInt();
	}


	// o baby!
	auto res = db_.AddAsset(modId, type, assName);
	if (res == assetDb::AssetDB::Result::OK) {
		done(QDialog::Accepted);
		return;
	}

	// fuck you
	if (res == assetDb::AssetDB::Result::NAME_TAKEN) {
		QMessageBox::warning(this, "Add Asset", "Failed to add asset, asset with same name and type already exsists", QMessageBox::Ok);
	}
	// fuck me
	else if (res == assetDb::AssetDB::Result::ERROR) {
		QMessageBox::critical(this, "Add Asset", "Error adding asset to db, check error log", QMessageBox::Ok);
	}
	else {
		QString msg = QString("Error adding asset. Unknown error(%1)").arg(QString::number(res));
		QMessageBox::critical(this, "Add Asset", msg, QMessageBox::Ok);
	}

	// make them click canel to close.
}

void AddAssetDialog::reject(void)
{
	done(QDialog::Rejected);
}


void AddAssetDialog::done(int32_t val)
{
	QDialog::done(val);
}



X_NAMESPACE_END