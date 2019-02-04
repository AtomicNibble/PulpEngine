#include "stdafx.h"
#include "AddAssetDialog.h"

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(assman)

AddAssetDialog::AddAssetDialog(QWidget* parent, assetDb::AssetDB& db) :
    QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
    db_(db),
    modId_(assetDb::AssetDB::INVALID_MOD_ID)
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

        QRegularExpression re(QString("[a-z0-9_\\%1\\%2]*").arg(QChar(assetDb::ASSET_NAME_PREFIX)).arg(QChar(assetDb::ASSET_NAME_SLASH)));

        pAssetName_->setMaxLength(assetDb::ASSET_NAME_MAX_LENGTH);
        pAssetName_->setValidator(new QRegularExpressionValidator(re));
        pAssetName_->setMinimumWidth(256);

        pFormLayout->addRow("Name", pAssetName_);
    }

    {
        QStringList items;

        // shiieeeet, auto populate it from the type enum.
        for (uint32_t i = 0; i < assetDb::AssetType::ENUM_COUNT; i++) {
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

        if (!db_.GetModsList(mods)) {
            pMod_->setDisabled(true);
            X_ERROR("AssetExplor", "Failed to iterate mods");
        }
        else {
            for (const auto& mod : mods) {
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

void AddAssetDialog::setAssetType(assetDb::AssetType::Enum type)
{
    const int32_t typeIndex = static_cast<int32_t>(type);

    BUG_ASSERT(typeIndex >= 0 && typeIndex < pAssetType_->count(), return );

    pAssetType_->setCurrentIndex(typeIndex);
    pAssetType_->setDisabled(true);

    QString typeStr(assetDb::AssetType::ToString(type));
    setWindowTitle(QString("Add '%1' Asset").arg(Utils::capitalize(typeStr)));
}

void AddAssetDialog::setPrefredMod(const QString& modName)
{
    const int32_t idx = pMod_->findText(modName, Qt::MatchExactly);
    if (idx >= 0) {
        pMod_->setCurrentIndex(idx);
    }
}

void AddAssetDialog::setNameHint(const QString& hint)
{
    pAssetName_->setText(hint);
}

core::string AddAssetDialog::getName(void) const
{
    return assetName_;
}

assetDb::AssetType::Enum AddAssetDialog::getType(void) const
{
    return type_;
}

int32_t AddAssetDialog::getModId(void) const
{
    return modId_;
}

void AddAssetDialog::accept(void)
{
    modId_ = assetDb::AssetDB::INVALID_MOD_ID;

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

        assetName_ = latinStr;
    }

    // type
    {
        const int32_t index = pAssetType_->currentIndex();

        if (index < 0 || index >= assetDb::AssetType::ENUM_COUNT) {
            QMessageBox::warning(this, "Invalid asset type", "Error getting asset type. (slap a dev)", QMessageBox::Ok);
            return;
        }

        type_ = static_cast<assetDb::AssetType::Enum>(index);
    }

    // mod
    {
        const int32_t index = pMod_->currentIndex();

        if (index < 0) {
            QMessageBox::critical(this, "Add Asset", "Invalid mod index", QMessageBox::Ok);
            return;
        }

        auto variant = pMod_->itemData(index);

        // make sure it's valid, as during inital testing it was not, giving me modId of zero.
        if (!variant.isValid()) {
            QMessageBox::critical(this, "Add Asset", "Failed to decode selected mod", QMessageBox::Ok);
            return;
        }

        modId_ = variant.toInt();
    }

    // o baby!
    auto res = db_.AddAsset(modId_, type_, assetName_);
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