#pragma once


#include <QObject>

X_NAMESPACE_BEGIN(editor)


class RawFileLoader : public QThread
{
	Q_OBJECT

public:
	typedef core::Array<uint8_t> DataArr;

public:
	RawFileLoader(core::MemoryArenaBase* arena, assetDb::AssetType::Enum type);
	~RawFileLoader();

	void loadFile(const QString& path);

	QString getPath(void) const;
	Vec2i getSrcDim(void) const;
	const DataArr& getCompressedSrc(void) const;
	const DataArr& getThumbData(void) const;

signals:
	void setProgress(int32_t pro);
	void setProgressLabel(const QString& label, int32_t pro);

	public slots:
	void stopProcess();

protected:
	void run();

private:
	assetDb::AssetType::Enum type_;

	QString path_;

	Vec2i srcDim_;
	DataArr compressed_;
	DataArr thumbData_;

	QMutex mutex_;
	bool abort_;
};


X_NAMESPACE_END