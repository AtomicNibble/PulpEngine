#pragma once

#include <QObject>


X_NAMESPACE_BEGIN(assman)



class IAssetEntry : public QObject
{
	Q_OBJECT

public:
	IAssetEntry(QObject *parent);
	~IAssetEntry();


	QString name(void) const;
	QString displayName(void) const;
	void setDisplayName(const QString& name);

	virtual bool isFileReadOnly(void) const;
	bool isTemporary(void) const;
	void setTemporary(bool temporary);

	virtual bool shouldAutoSave(void) const;
	virtual bool isModified(void) const X_ABSTRACT;
	virtual bool isSaveAsAllowed(void) const X_ABSTRACT;

	bool autoSave(QString* pErrorString, const QString& fileName);

signals:
	void changed(void);
	void aboutToReload(void);
	void reloadFinished(bool success);


private:
	QString displayName_;
	bool temporary_;

};

X_NAMESPACE_END