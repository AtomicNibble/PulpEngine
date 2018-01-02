#pragma once

#include <QObject>

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(editor)


class RenameAssetDialog : public QDialog
{
	Q_OBJECT

public:
	RenameAssetDialog(QWidget *parent, assetDb::AssetDB& db, assetDb::AssetType::Enum type, const QString& name);
	~RenameAssetDialog();

private:
	void createGui(const QString& name);

private slots:
	void accept(void);
	void reject(void);
	void done(int32_t val);


private:
	assetDb::AssetDB& db_;

	assetDb::AssetType::Enum type_;
	core::string origName_;
	QLineEdit* pAssetName_;
};





X_NAMESPACE_END