#pragma once

#include <QObject>

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(assman)


class AddAssetDialog : public QDialog
{
    Q_OBJECT

    using StrArr = core::Array<core::string>;
    using AssetNameArr = StrArr;

    struct BraceExpansion
    {
        BraceExpansion(core::MemoryArenaBase* arena) :
            values(arena)
        {}

        core::string base;
        StrArr values;
    };

    using BraceArr = core::Array<BraceExpansion>;

public:
	AddAssetDialog(QWidget *parent, assetDb::AssetDB& db);
	~AddAssetDialog();

	void setAssetType(assetDb::AssetType::Enum type);
	void setPrefredMod(const QString& modName);
	void setNameHint(const QString& hint);

	const AssetNameArr& getNames(void) const;
	assetDb::AssetType::Enum getType(void) const;
	int32_t getModId(void) const;

private slots:
	void accept(void);
	void reject(void);
	void done(int32_t val);

private:
    void expandName(const core::string& assetNamename);
    void addSubBraces(const BraceArr& braces, core::string& name, size_t idx);

private:
	assetDb::AssetDB& db_;

	QLineEdit* pAssetName_;
	QComboBox* pAssetType_;
	QComboBox* pMod_;

private:
    AssetNameArr assetNames_;
	assetDb::AssetType::Enum type_;
	int32_t modId_;
};





X_NAMESPACE_END