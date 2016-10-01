#pragma once

#include <QObject>


X_NAMESPACE_BEGIN(assman)



class IAssetEntry : public QObject
{
	Q_OBJECT

public:
	IAssetEntry(QObject *parent);
	~IAssetEntry();

	virtual bool save(QString& errorString) X_ABSTRACT;

	QString name(void) const;
	QString displayName(void) const;
	void setAssetName(const QString& name);
	assetDb::AssetType::Enum type(void) const;
	void setType(assetDb::AssetType::Enum type);


	virtual bool isFileReadOnly(void) const;
	bool isTemporary(void) const;
	void setTemporary(bool temporary);

	virtual bool shouldAutoSave(void) const;
	virtual bool isModified(void) const X_ABSTRACT;
	virtual bool isSaveAsAllowed(void) const X_ABSTRACT;

	bool autoSave(QString* pErrorString);

signals:
	void changed(void);
	void aboutToReload(void);
	void reloadFinished(bool success);


protected:
	QString assetName_;
	assetDb::AssetType::Enum type_;
	bool temporary_;

};

X_NAMESPACE_END