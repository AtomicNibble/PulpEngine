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
	static QList<IAssetEntry*> modifiedAssetEntrys(void);

	static void renamedFile(const QString& from, const QString& to);

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
	void currentFileChanged(const QString& filePath);
	// emitted if one AssetEntry changed its name e.g. due to save as
	void assetEntryRenamed(IAssetEntry* pAssetEntry, const QString& from, const QString& to);


private slots:
	void assetEntryDestroyed(QObject* pObj);
	void syncWithEditor(const QList<IContext *>& context);


private:
	static bool saveModifiedFilesHelper(const QList<IAssetEntry*>& assetEntrys,
		const QString& message, bool* pCancelled, bool silently,
		const QString& alwaysSaveMessage, bool* pAlwaysSave,
		QList<IAssetEntry*>* pFailedToSave);

};





X_NAMESPACE_END