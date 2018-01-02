#pragma once

#include <QObject>

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(editor)


class DeleteAssetDialog : public QDialog
{
	Q_OBJECT

public:
	DeleteAssetDialog(QWidget *parent, assetDb::AssetDB& db, assetDb::AssetType::Enum type, const QString& name);
	~DeleteAssetDialog();

	// i made the loading of info a seperator function and not in hte constructor as it can fail.
	// I pondered the idea of having the constructor take the pre quered data
	// but that would mean anyone wanting to delete something would have to write the 
	// same query logic before creating this dialog, so since the logic is related 
	// to using this dialog i placed it here, to reducde duplication.
	// 
	// one idea might be to make this memeber staitc tho and pass data to constructor tho....
	bool loadInfo(void);

	core::string getName(void) const;
	assetDb::AssetType::Enum getType(void) const;

private:

private slots:
	void accept(void);
	void reject(void);
	void done(int32_t val);


private:
	assetDb::AssetDB& db_;
	assetDb::AssetType::Enum type_;
	core::string name_;

	QFormLayout* pFormLayout_;
	QLineEdit* pAssetName_;
	QListWidget* pRefList_;
	QDialogButtonBox* pButtonBox_;

	bool infoLoaded_;
	bool hasRefs_;
};





X_NAMESPACE_END