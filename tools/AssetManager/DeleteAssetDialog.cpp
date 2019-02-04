#include "stdafx.h"
#include "DeleteAssetDialog.h"

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(assman)

DeleteAssetDialog::DeleteAssetDialog(QWidget* parent, assetDb::AssetDB& db, assetDb::AssetType::Enum type, const QString& name) :
    QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
    db_(db),
    type_(type),
    name_(name.toLatin1()),
    infoLoaded_(false),
    hasRefs_(false)
{
    setWindowTitle("Delete Asset - Confirm");

    pAssetName_ = new QLineEdit();
    pAssetName_->setReadOnly(true);
    pAssetName_->setText(name);

    pRefList_ = new QListWidget(this);

    pFormLayout_ = new QFormLayout();
    pFormLayout_->addRow("Name", pAssetName_);
    pFormLayout_->addRow("Refrenced by", pRefList_);

    {
        pButtonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        pButtonBox_->button(QDialogButtonBox::Cancel)->setDefault(true);
        connect(pButtonBox_, SIGNAL(accepted()), this, SLOT(accept()));
        connect(pButtonBox_, SIGNAL(rejected()), this, SLOT(reject()));
        pFormLayout_->addWidget(pButtonBox_);
    }

    setLayout(pFormLayout_);
}

DeleteAssetDialog::~DeleteAssetDialog()
{
}

bool DeleteAssetDialog::loadInfo(void)
{
    // get asset id.
    int32_t assetId;
    if (!db_.AssetExists(type_, name_, &assetId)) {
        X_ERROR("DeleteAsset", "Can't delete asset it does not exsist.");
        return false;
    }

    assetDb::AssetDB::AssetIdArr refs(g_arena);
    if (!db_.GetAssetRefs(assetId, refs)) {
        X_ERROR("DeleteAsset", "Failed to get asset refs");
        return false;
    }

    if (refs.isNotEmpty()) {
        hasRefs_ = true;

        // disable ok.
        pButtonBox_->button(QDialogButtonBox::Ok)->setEnabled(false);
        pButtonBox_->button(QDialogButtonBox::Ok)->setToolTip(tr("Can't delete asset with refrences"));

        assetDb::AssetDB::AssetInfo info;
        for (const auto& ref : refs) {
            if (!db_.GetAssetInfoForAsset(ref, info)) {
                X_ERROR("DeleteAsset", "Failed to get asset refs info");
                return false;
            }

            QString name = QString::fromLocal8Bit(info.name.c_str(), static_cast<int32_t>(info.name.length()));
            pRefList_->addItem(name);
        }
    }
    else {
        pRefList_->setVisible(false);
        QWidget* pLabel = pFormLayout_->labelForField(pRefList_);
        if (pLabel) {
            pLabel->hide();
        }
    }

    infoLoaded_ = true;
    return true;
}

core::string DeleteAssetDialog::getName(void) const
{
    return name_;
}

assetDb::AssetType::Enum DeleteAssetDialog::getType(void) const
{
    return type_;
}

void DeleteAssetDialog::accept(void)
{
    if (!infoLoaded_) {
        QMessageBox::critical(this, "Delete Asset", "Source code error, asset info not loaded.", QMessageBox::Ok);
        return;
    }

    auto res = db_.DeleteAsset(type_, name_);
    if (res == assetDb::AssetDB::Result::OK) {
        done(QDialog::Accepted);
        return;
    }

    if (res == assetDb::AssetDB::Result::ERROR) {
        QMessageBox::critical(this, "Delete Asset", "Error deleting asset, check error log", QMessageBox::Ok);
    }
    else {
        QString msg = QString("Error deleting asset. Unknown error(%1)").arg(QString::number(res));
        QMessageBox::critical(this, "Delete Asset", msg, QMessageBox::Ok);
    }

    // make them click canel to close.
}

void DeleteAssetDialog::reject(void)
{
    done(QDialog::Rejected);
}

void DeleteAssetDialog::done(int32_t val)
{
    QDialog::done(val);
}

X_NAMESPACE_END