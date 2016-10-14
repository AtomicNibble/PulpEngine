#pragma once

#include <QObject>

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);


X_NAMESPACE_BEGIN(assman)

class AssetAssetRefWidget : public QWidget
{
	Q_OBJECT

public:
	AssetAssetRefWidget(QWidget *parent, assetDb::AssetDB& db, assetDb::AssetType::Enum type, const std::string& value);
	~AssetAssetRefWidget();

signals:
	void valueChanged(const std::string& value);

private slots:
	void setValue(const std::string& value);
	void browseClicked(void);

private:
	QLineEdit* pLineEdit_;

private:
	assetDb::AssetDB& db_;
	assetDb::AssetType::Enum type_;
};

X_NAMESPACE_END