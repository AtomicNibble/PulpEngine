#include "stdafx.h"
#include "RenameAssetDialog.h"

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(assman)

RenameAssetDialog::RenameAssetDialog(QWidget* parent, assetDb::AssetDB& db, assetDb::AssetType::Enum type, const QString& name) :
    QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
    db_(db),
    type_(type)
{
    origName_ = name.toLatin1();
    createGui(name);
}

RenameAssetDialog::~RenameAssetDialog()
{
}

void RenameAssetDialog::createGui(const QString& name)
{
    setWindowTitle("Rename Asset");

    QFormLayout* pFormLayout = new QFormLayout();

    {
        pAssetName_ = new QLineEdit();

        QRegularExpression re(QString("[a-z0-9_\\%1\\%2]*").arg(QChar(assetDb::ASSET_NAME_PREFIX)).arg(QChar(assetDb::ASSET_NAME_SLASH)));

        pAssetName_->setMaxLength(assetDb::ASSET_NAME_MAX_LENGTH);
        pAssetName_->setValidator(new QRegularExpressionValidator(re));
        pAssetName_->setText(name);
        pAssetName_->setMinimumWidth(256);

        pFormLayout->addRow("Name", pAssetName_);
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

void RenameAssetDialog::accept(void)
{
    core::string newName;

    // name
    {
        // my nuggger.
        const QString name = pAssetName_->text();

        if (name.isEmpty()) {
            QMessageBox::warning(this, "Invalid asset name", "New Asset name is empty", QMessageBox::Ok);
            return;
        }
        if (name.length() < assetDb::ASSET_NAME_MIN_LENGTH) {
            QMessageBox::warning(this, "Invalid asset name", "New Asset name is too short", QMessageBox::Ok);
            return;
        }

        newName = name.toLatin1();
    }

    if (origName_ == newName) {
        QMessageBox::information(this, "Rename Asset", "Name is unchanged", QMessageBox::Ok);
        return;
    }

    auto res = db_.RenameAsset(type_, origName_, newName);
    if (res == assetDb::AssetDB::Result::OK) {
        done(QDialog::Accepted);
        return;
    }

    if (res == assetDb::AssetDB::Result::NAME_TAKEN) {
        QMessageBox::warning(this, "Rename Asset", "Failed to rename asset, asset with same name and type already exsists", QMessageBox::Ok);
    }
    else if (res == assetDb::AssetDB::Result::ERROR) {
        QMessageBox::critical(this, "Rename Asset", "Error renaming asset to db, check error log", QMessageBox::Ok);
    }
    else {
        QString msg = QString("Error renaming asset. Unknown error(%1)").arg(QString::number(res));
        QMessageBox::critical(this, "Rename Asset", msg, QMessageBox::Ok);
    }

    // make them click canel to close.
}

void RenameAssetDialog::reject(void)
{
    done(QDialog::Rejected);
}

void RenameAssetDialog::done(int32_t val)
{
    QDialog::done(val);
}

X_NAMESPACE_END