#pragma once

#include <QObject>

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);


X_NAMESPACE_BEGIN(assman)

class IAssetEntry;

class AssetAssetRefWidget : public QWidget
{
	Q_OBJECT

public:
	AssetAssetRefWidget(QWidget *parent, assetDb::AssetDB& db, IAssetEntry* pAssEntry, 
		const std::string& typeStr, const std::string& value);
	~AssetAssetRefWidget();

signals:
	void valueChanged(const std::string& value);

private slots:
	void setValue(const std::string& value);
    void clearClicked(void);
    void browseClicked(void);

private:
	bool removeRef(int32_t assetId, const QString& assName);
	bool addRef(int32_t assetId, const QString& assName);


private:
	QLineEdit* pLineEdit_;

private:
	assetDb::AssetDB& db_;
	IAssetEntry* pAssEntry_;
	assetDb::AssetType::Enum type_;
};

X_NAMESPACE_END