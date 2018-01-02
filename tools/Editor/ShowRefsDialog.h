#pragma once

#include <QObject>

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(editor)


class AssetRefsDialog : public QDialog
{
	Q_OBJECT

public:
	AssetRefsDialog(QWidget *parent, assetDb::AssetDB& db, assetDb::AssetType::Enum type, const QString& name);
	~AssetRefsDialog();


	bool loadInfo(void);

private:

	private slots :
		void accept(void);
	void reject(void);
	void done(int32_t val);


private:
	assetDb::AssetDB& db_;
	assetDb::AssetType::Enum type_;
	core::string name_;

	QFormLayout* pFormLayout_;
	QListWidget* pRefList_;

	bool infoLoaded_;
};





X_NAMESPACE_END