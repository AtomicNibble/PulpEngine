#pragma once

#include <QObject>

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(assman)


class AddAssetDialog : public QDialog
{
	Q_OBJECT

public:
	AddAssetDialog(QWidget *parent, assetDb::AssetDB& db);
	~AddAssetDialog();

	void setAssetType(assetDb::AssetType::Enum type);
	void setPrefredMod(const QString& modName);
	void setNameHint(const QString& hint);

	core::string getName(void) const;
	assetDb::AssetType::Enum getType(void) const;
	int32_t getModId(void) const;

private slots:
	void accept(void);
	void reject(void);
	void done(int32_t val);


private:
	assetDb::AssetDB& db_;

	QLineEdit* pAssetName_;
	QComboBox* pAssetType_;
	QComboBox* pMod_;

private:
	core::string assetName_;
	assetDb::AssetType::Enum type_;
	int32_t modId_;
};





X_NAMESPACE_END