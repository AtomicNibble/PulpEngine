#include "stdafx.h"
#include "AddAssetDialog.h"

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(assman)

AddAssetDialog::AddAssetDialog(QWidget* parent, assetDb::AssetDB& db) :
    QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
    db_(db),
    modId_(assetDb::AssetDB::INVALID_MOD_ID),
    assetNames_(g_arena)
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

        QRegularExpression re(QString("[a-z0-9_\\%1\\%2{,}.]*").arg(QChar(assetDb::ASSET_NAME_PREFIX)).arg(QChar(assetDb::ASSET_NAME_SLASH)));

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

const AddAssetDialog::AssetNameArr& AddAssetDialog::getNames(void) const
{
    return assetNames_;
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
    core::string assetName;
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

        assetName = name.toLatin1();
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

    expandName(assetName);

    if (assetNames_.isEmpty()) {
        QMessageBox::critical(this, "Add Asset", "Failed to parse assetname", QMessageBox::Ok);
        return;
    }

    for (auto& assetName : assetNames_)
    {
        // o baby!
        auto res = db_.AddAsset(modId_, type_, assetName);
        if (res == assetDb::AssetDB::Result::OK) {
            continue;
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

        // make them close it for errors.
        return;
    }

    done(QDialog::Accepted);
}

void AddAssetDialog::reject(void)
{
    done(QDialog::Rejected);
}

void AddAssetDialog::done(int32_t val)
{
    QDialog::done(val);
}

void AddAssetDialog::expandName(const core::string& assetName)
{
    if (!assetName.find('{')) {
        assetNames_.emplace_back(assetName);
        return;
    }

    // i support expansion so can create multiple assets at once.
    // currently i think it will just be numeric ranges and comma seperalted string list.
    // {0..1}_{goat,boat,moat} 
    // Results in:
    //  0_goat
    //  0_boat
    //  0_moat
    //  1_goat
    //  1_boat
    //  1_moat


    BraceArr braces(g_arena);

    auto* pBeginBrace = assetName.begin();
    auto* pNameBegin = pBeginBrace;
    while ((pBeginBrace = core::strUtil::Find(pBeginBrace, assetName.end(), '{')) != nullptr) {
        auto* pEndBrace = core::strUtil::Find(pBeginBrace, assetName.end(), '}');
        if (!pEndBrace) {
            QMessageBox::critical(this, "Add Asset", "Brace expansion missing closing brace }", QMessageBox::Ok);
            return;
        }

        auto* pBegin = pBeginBrace + 1;
        auto* pEnd = pEndBrace;

        if (pBegin >= pEnd) {
            QMessageBox::critical(this, "Add Asset", "Brace expansion empty", QMessageBox::Ok);
            return;
        }

        auto& brace = braces.AddOne(g_arena);
        brace.base.assign(pNameBegin, pBeginBrace);

        auto* pEntryBegin = pBegin;
        auto* pEntryEnd = pBegin;

        do {
            // now look for commas.
            auto* pNextComma = core::strUtil::Find(pEntryBegin, pEnd, ',');
            if (pNextComma) {
                pEntryEnd = pNextComma;
            }
            else {
                pEntryEnd = pEnd;
            }

            core::string value(pEntryBegin, pEntryEnd);
            if (auto* pDots = value.find(".."); pDots) {
                // get start end and generate all the shit.
                int32_t start = core::strUtil::StringToInt<int32_t>(value.begin(), pDots);
                int32_t end = core::strUtil::StringToInt<int32_t>(pDots + 2, value.end());

                if (start >= end) {
                    QMessageBox::critical(this, "Add Asset", "Brace expansion invalid int range", QMessageBox::Ok);
                    return;
                }

                while (start <= end) {
                    core::StackString<64> str(start);

                    brace.values.emplace_back(str.begin(), str.end());
                    ++start;
                }
            }
            else {
                brace.values.emplace_back(std::move(value));
            }

            // move to next one.
            pEntryBegin = pEntryEnd;
            if (pNextComma) {
                pEntryBegin = pNextComma + 1;
            }

        } while (pEntryEnd != pEnd);

        pBeginBrace = pEndBrace + 1;
        pNameBegin = pBeginBrace;
    }

    if (braces.isEmpty()) {
        QMessageBox::critical(this, "Add Asset", "Brace expansion failed", QMessageBox::Ok);
        return;
    }

    addSubBraces(braces, core::string(), 0);
}

void AddAssetDialog::addSubBraces(const BraceArr& braces, core::string& name, size_t idx)
{
    auto& brace = braces[idx];
    auto isLast = idx + 1 == braces.size();

    // for each value.
    for (size_t i = 0; i < brace.values.size(); i++)
    {
        auto& value = brace.values[i];
        core::string subName = name + brace.base + value;

        if (isLast) {
            assetNames_.append(subName);
        }
        else {
            addSubBraces(braces, subName, idx + 1);
        }
    }
}


X_NAMESPACE_END