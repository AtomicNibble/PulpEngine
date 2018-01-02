#pragma once

#include <QObject>


X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(assman)

class AddModDialog : public QDialog
{
	Q_OBJECT

public:
	AddModDialog(QWidget *parent, assetDb::AssetDB& db);
	~AddModDialog();

private slots:
	void accept(void);
	void reject(void);
	void done(int32_t val);


private:
	assetDb::AssetDB& db_;

	QLineEdit* pModName_;
	QLineEdit* pOutDir_;
};





X_NAMESPACE_END