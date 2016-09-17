#pragma once

#include <QObject>


X_NAMESPACE_BEGIN(assman)

class IAssetEntry;

class AssetEntryManager : public QObject
{
	Q_OBJECT
public:

	typedef QPair<QString, Id> RecentFile;

public:
	explicit AssetEntryManager(QObject *parent);
	~AssetEntryManager();

public:

	static QObject* instance(void);

	// file pool to monitor
	static void addAssetEntry(const QList<IAssetEntry*>& assetEntrys);
	static void addAssetEntry(IAssetEntry* pAssetEntry);
	static bool removeAssetEntry(IAssetEntry* pAssetEntry);
	static QList<IAssetEntry*> modifiedAssetEntry(void);

	static void renamedFile(const QString& from, const QString& to);

	static void expectFileChange(const QString& fileName);
	static void unexpectFileChange(const QString& fileName);

	// recent files
	static void addToRecentFiles(const QString& fileName, const Id &editorId = Id());
	Q_SLOT void clearRecentFiles(void);
	static QList<RecentFile> recentFiles(void);


//	static void saveSettings(void);

	// current file
	static void setCurrentFile(const QString& fileName);
	static QString currentFile(void);


	// helper functions
	static QString fixFileName(const QString& fileName);

	static bool saveAssetEntry(IAssetEntry* pAssetEntry, const QString& fileName = QString(), bool *isReadOnly = nullptr);

	static QString getSaveFileName(const QString& title, const QString& pathIn,
		const QString& filter = QString(), QString* selectedFilter = nullptr);
	static QString getSaveFileNameWithExtension(const QString& title, const QString& pathIn,
		const QString& filter);
	static QString getSaveAsFileName(const IAssetEntry* pAssetEntry, const QString& filter = QString(),
		QString* pSelectedFilter = nullptr);


	static bool saveAllModifiedAssetEntrys(const QString& message = QString(), bool *canceled = nullptr,
		const QString& alwaysSaveMessage = QString(),
		bool *alwaysSave = nullptr,
		QList<IAssetEntry *> *failedToClose = nullptr);
	static bool saveModifiedAssetEntrys(const QList<IAssetEntry *>& assetEntrys,
		const QString& message = QString(), bool *canceled = nullptr,
		const QString& alwaysSaveMessage = QString(),
		bool *alwaysSave = nullptr,
		QList<IAssetEntry *> *failedToClose = nullptr);
	static bool saveModifiedAssetEntry(IAssetEntry *pAssetEntry,
		const QString& message = QString(), bool *canceled = nullptr,
		const QString& alwaysSaveMessage = QString(),
		bool *alwaysSave = nullptr,
		QList<IAssetEntry *> *failedToClose = nullptr);


signals:
	void currentFileChanged(const QString& filePath);
	// emitted if one AssetEntry changed its name e.g. due to save as
	void assetEntryRenamed(IAssetEntry* pAssetEntry, const QString& from, const QString& to);


private slots:
	void assetEntryDestroyed(QObject* pObj);
	void syncWithEditor(const QList<IContext *>& context);


};





X_NAMESPACE_END