#pragma once

#include <QObject>


X_NAMESPACE_BEGIN(assman)

class IAssetEntry;

class AssetEntryManager : public QObject
{
	Q_OBJECT
public:

	struct RecentAsset
	{
		RecentAsset() = default;
		RecentAsset(QString name, assetDb::AssetType::Enum type, Id id);

		bool operator==(const RecentAsset& oth) const;

		QString name;
		assetDb::AssetType::Enum type;
		Id id;
	};

	struct AssetInfo
	{
		AssetInfo() = default;
		AssetInfo(QString name, assetDb::AssetType::Enum type);

		bool operator==(const AssetInfo& oth) const;

		QString name;
		assetDb::AssetType::Enum type;
	};

	// typedef QPair<QString, Id> RecentFile;

public:
	explicit AssetEntryManager(QObject *parent);
	~AssetEntryManager();

public:

	static QObject* instance(void);

	// file pool to monitor
	static void addAssetEntry(const QList<IAssetEntry*>& assetEntrys);
	static void addAssetEntry(IAssetEntry* pAssetEntry);
	static bool removeAssetEntry(IAssetEntry* pAssetEntry);
	static QList<IAssetEntry*> modifiedAssetEntrys(void);

	// recent files
	static void addToRecentFiles(const QString& fileName, assetDb::AssetType::Enum type, const Id &editorId);
	Q_SLOT void clearRecentFiles(void);
	static QList<RecentAsset> recentAssets(void);

	// reload UI
	static void reloadUIforType(assetDb::AssetType::Enum type);

	// current file
	static void setCurrentFile(const QString& fileName, assetDb::AssetType::Enum type);
	static AssetInfo currentFile(void);

	// helper functions
	static bool saveAssetEntry(IAssetEntry* pAssetEntry);


	static bool saveAllModifiedAssetEntrysSilently(bool* pCanceled = nullptr,
		QList<IAssetEntry*>* pFailedToClose = nullptr);
	static bool saveModifiedAssetEntrysSilently(const QList<IAssetEntry *>& assetEntrys, bool* pCanceled = nullptr,
		QList<IAssetEntry*>* pFailedToClose = nullptr);
	static bool saveModifiedAssetEntrySilently(IAssetEntry* pAssetEntry, bool* pCanceled = nullptr,
		QList<IAssetEntry*>* pFailedToClose = nullptr);


	static bool saveAllModifiedAssetEntrys(const QString& message = QString(), bool* pCanceled = nullptr,
		const QString& alwaysSaveMessage = QString(),
		bool* pAlwaysSave = nullptr,
		QList<IAssetEntry *> *pFailedToClose = nullptr);
	static bool saveModifiedAssetEntrys(const QList<IAssetEntry *>& assetEntrys,
		const QString& message = QString(), bool* pCanceled = nullptr,
		const QString& alwaysSaveMessage = QString(),
		bool* pAlwaysSave = nullptr,
		QList<IAssetEntry *> *pFailedToClose = nullptr);
	static bool saveModifiedAssetEntry(IAssetEntry* pAssetEntry,
		const QString& message = QString(), bool* pCanceled = nullptr,
		const QString& alwaysSaveMessage = QString(),
		bool* pAlwaysSave = nullptr,
		QList<IAssetEntry *>* pFailedToClose = nullptr);


signals:
	void currentFileChanged(const QString& name, assetDb::AssetType::Enum type);


private slots:
	void assetEntryDestroyed(QObject* pObj);
	void syncWithEditor(const QList<IContext *>& context);


private:
	static bool saveModifiedFilesHelper(const QList<IAssetEntry*>& assetEntrys,
		const QString& message, bool* pCancelled, bool silently,
		const QString& alwaysSaveMessage, bool* pAlwaysSave,
		QList<IAssetEntry*>* pFailedToSave);

};


Q_DECLARE_METATYPE(AssetEntryManager::RecentAsset)


X_NAMESPACE_END