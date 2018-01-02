#pragma once
#pragma once

#include <QObject>

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(editor)


class SelectAssetDialog : public QDialog
{
	Q_OBJECT

public:
	SelectAssetDialog(QWidget *parent, assetDb::AssetDB& db, assetDb::AssetType::Enum type);
	~SelectAssetDialog();

	QString getSelectedName(void) const;

private:

private slots:
	void processSelected(const QModelIndex & index);
	void accept(void);
	void reject(void);
	void done(int32_t val);


private:
	assetDb::AssetDB& db_;
	assetDb::AssetType::Enum type_;

	QStringListModel items_;
	QSortFilterProxyModel* pSortFilter_;

	QLineEdit* pSearch_;
	QListView* pList_;

	QString selectedAssetName_;
};





X_NAMESPACE_END