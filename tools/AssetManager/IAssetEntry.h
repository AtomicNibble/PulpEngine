#pragma once

#include <QObject>

#include <ICompression.h>

X_NAMESPACE_BEGIN(assman)



class IAssetEntry : public QObject
{
	Q_OBJECT

public:
	typedef core::Array<uint8_t> ByteArr;

public:
	IAssetEntry(QObject *parent);
	~IAssetEntry();

	virtual bool save(QString& errorString) X_ABSTRACT;
	virtual bool updateRawFile(const ByteArr& compressedData) X_ABSTRACT;
	virtual bool getRawFile(ByteArr& rawData) X_ABSTRACT;
	virtual bool updateThumb(const ByteArr& data, Vec2i thumbDim, Vec2i srcDim,
		core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl) X_ABSTRACT;
	virtual bool getThumb(core::Array<uint8_t>& data, Vec2i& dim) X_ABSTRACT;
	virtual bool reloadUi(void) X_ABSTRACT;

	QString name(void) const;
	core::string nameNarrow(void) const;
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