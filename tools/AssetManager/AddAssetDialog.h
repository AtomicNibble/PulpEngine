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

private slots:
	void accept(void);
	void reject(void);
	void done(int32_t val);


private:
	assetDb::AssetDB& db_;

	QLineEdit* pAssetName_;
	QComboBox* pAssetType_;
	QComboBox* pMod_;
};





X_NAMESPACE_END