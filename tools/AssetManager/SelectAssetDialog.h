#pragma once
#pragma once

#include <QObject>

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(assman)


class SelectAssetDialog : public QDialog
{
	Q_OBJECT

public:
	SelectAssetDialog(QWidget *parent, assetDb::AssetDB& db, assetDb::AssetType::Enum type);
	~SelectAssetDialog();

	QString getSelectedName(void) const;

private slots:
	void accept(void);
	void reject(void);
	void done(int32_t val);


private:
	assetDb::AssetDB& db_;
	assetDb::AssetType::Enum type_;

	QListWidget* pList_;

	QString selectedAssetName_;
};





X_NAMESPACE_END