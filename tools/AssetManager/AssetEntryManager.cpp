#include "stdafx.h"
#include "AssetEntryManager.h"

#include "IAssetEntry.h"
#include "IEditor.h"
#include "EditorManager.h"

X_NAMESPACE_BEGIN(assman)

namespace
{

	struct FileStateItem
	{
		QDateTime modified;
		QFile::Permissions permissions;
	};

	struct FileState
	{
		QMap<IAssetEntry *, FileStateItem> lastUpdatedState;
		FileStateItem expected;
	};

	struct AssetEntryManagerPrivate
	{
		AssetEntryManagerPrivate();

		static const int32_t MMAX_RECENT_FILES = 7;

	public:
		QMap<QString, FileState> states_;
		QSet<QString> changedFiles_;
		QSet<QString> expectedFileNames_;
		QList<IAssetEntry*> assetEntrys_;

		QList<AssetEntryManager::RecentFile> recentFiles_;

		QString currentFile_;
	};


	AssetEntryManagerPrivate::AssetEntryManagerPrivate() 
	{

	}

	static AssetEntryManager* pInstance_ = nullptr;
	static AssetEntryManagerPrivate* d = nullptr;


} // namespace

AssetEntryManager::AssetEntryManager(QObject *parent) :
	QObject(parent)
{
	d = new AssetEntryManagerPrivate;
	pInstance_ = this;

	connect(ICore::instance(), SIGNAL(contextChanged(QList<IContext*>, Context)),
		this, SLOT(syncWithEditor(QList<IContext*>)));
}

AssetEntryManager::~AssetEntryManager()
{

}


QObject* AssetEntryManager::instance(void)
{
	return pInstance_;
}


// file pool to monitor
void AssetEntryManager::addAssetEntry(const QList<IAssetEntry*>& assetEntrys)
{
	foreach(IAssetEntry* pAssetEntry, assetEntrys)
	{
		if (pAssetEntry && !d->assetEntrys_.contains(pAssetEntry))
		{
			connect(pAssetEntry, SIGNAL(destroyed(QObject*)), pInstance_, SLOT(assetEntryDestroyed(QObject*)));
			connect(pAssetEntry, SIGNAL(filePathChanged(QString, QString)), pInstance_, SLOT(filePathChanged(QString, QString)));
			d->assetEntrys_.append(pAssetEntry);
		}
	}
	return;
}

void AssetEntryManager::addAssetEntry(IAssetEntry* pAssetEntry)
{
	addAssetEntry(QList<IAssetEntry *>() << pAssetEntry);
}

bool AssetEntryManager::removeAssetEntry(IAssetEntry* pAssetEntry)
{
	BUG_ASSERT(pAssetEntry, return false);

	if (!d->assetEntrys_.removeOne(pAssetEntry))
	{
	//	removeFileInfo(pAssetEntry);
		disconnect(pAssetEntry, SIGNAL(changed()), pInstance_, SLOT(checkForNewFileName()));
	}

	disconnect(pAssetEntry, SIGNAL(destroyed(QObject*)), pInstance_, SLOT(assetEntryDestroyed(QObject*)));

	return true;
}

QList<IAssetEntry*> AssetEntryManager::modifiedAssetEntrys(void)
{
	QList<IAssetEntry *> modified;

	for(IAssetEntry* pAssetEntry : d->assetEntrys_) {
		if (pAssetEntry->isModified()) {
			modified << pAssetEntry;
		}
	}

	return modified;
}


void AssetEntryManager::renamedFile(const QString& from, const QString& to)
{
	X_UNUSED(from);
	X_UNUSED(to);
	X_ASSERT_NOT_IMPLEMENTED();
}



// recent files
void AssetEntryManager::addToRecentFiles(const QString& fileName, const Id& editorId)
{
	if (fileName.isEmpty()) {
		return;
	}

	QString unifiedForm(fixFileName(fileName));
	QMutableListIterator<RecentFile > it(d->recentFiles_);
	while (it.hasNext())
	{
		RecentFile file = it.next();
		QString recentUnifiedForm(fixFileName(file.first));
		if (unifiedForm == recentUnifiedForm) {
			it.remove();
		}
	}

	if (d->recentFiles_.count() > d->MMAX_RECENT_FILES) {
		d->recentFiles_.removeLast();
	}

	d->recentFiles_.prepend(RecentFile(fileName, editorId));
}

void AssetEntryManager::clearRecentFiles(void)
{
	d->recentFiles_.clear();
}

QList<AssetEntryManager::RecentFile> AssetEntryManager::recentFiles(void)
{
	return d->recentFiles_;
}



// current file
void AssetEntryManager::setCurrentFile(const QString& fileName)
{
	if (d->currentFile_ == fileName) {
		return;
	}
	d->currentFile_ = fileName;
	emit pInstance_->currentFileChanged(d->currentFile_);
}

QString AssetEntryManager::currentFile(void)
{
	return d->currentFile_;
}

// helper functions
QString AssetEntryManager::fixFileName(const QString& fileName)
{
	return fileName;
}


bool AssetEntryManager::saveAssetEntry(IAssetEntry* pAssetEntry, const QString& fileName, bool *isReadOnly)
{
	X_UNUSED(pAssetEntry);
	X_UNUSED(fileName);
	X_UNUSED(isReadOnly);

	return false;
}


QString AssetEntryManager::getSaveFileName(const QString& title, const QString& pathIn,
	const QString& filter, QString* selectedFilter)
{
	X_ASSERT_NOT_IMPLEMENTED();
	X_UNUSED(title);
	X_UNUSED(pathIn);
	X_UNUSED(filter);
	X_UNUSED(selectedFilter);

	return QString();
}



QString AssetEntryManager::getSaveAsFileName(const IAssetEntry* pAssetEntry, const QString& filter,
	QString* pSelectedFilter)
{
	X_ASSERT_NOT_IMPLEMENTED();
	X_UNUSED(pAssetEntry);
	X_UNUSED(filter);
	X_UNUSED(pSelectedFilter);

	return QString();
}


bool AssetEntryManager::saveAllModifiedAssetEntrysSilently(bool* pCanceled,
	QList<IAssetEntry*>* pFailedToClose)
{
	return saveModifiedAssetEntrysSilently(modifiedAssetEntrys(), pCanceled, pFailedToClose);
}

bool AssetEntryManager::saveModifiedAssetEntrysSilently(const QList<IAssetEntry*>& assetEntrys, bool* pCanceled,
	QList<IAssetEntry*>* pFailedToClose)
{
	return saveModifiedFilesHelper(assetEntrys, QString(), pCanceled, true, QString(), 0, pFailedToClose);
}

bool AssetEntryManager::saveModifiedAssetEntrySilently(IAssetEntry* pAssetEntry, bool* pCanceled,
	QList<IAssetEntry*>* pFailedToClose)
{
	return saveModifiedAssetEntrysSilently(QList<IAssetEntry*>() << pAssetEntry, pCanceled, pFailedToClose);
}


bool AssetEntryManager::saveAllModifiedAssetEntrys(const QString& message, bool *canceled,
	const QString& alwaysSaveMessage,
	bool *alwaysSave,
	QList<IAssetEntry *>* failedToClose )
{
	return saveModifiedAssetEntrys(modifiedAssetEntrys(), message, canceled,
		alwaysSaveMessage, alwaysSave, failedToClose);
}

bool AssetEntryManager::saveModifiedAssetEntrys(const QList<IAssetEntry*>& assetEntrys,
	const QString& message, bool *canceled,
	const QString& alwaysSaveMessage,
	bool *alwaysSave,
	QList<IAssetEntry *> *failedToClose)
{
	return saveModifiedFilesHelper(assetEntrys, message, canceled, false,
		alwaysSaveMessage, alwaysSave, failedToClose);
}

bool AssetEntryManager::saveModifiedAssetEntry(IAssetEntry* pAssetEntry,
	const QString& message, bool *canceled,
	const QString& alwaysSaveMessage,
	bool *alwaysSave,
	QList<IAssetEntry *> *failedToClose)
{
	return saveModifiedAssetEntrys(QList<IAssetEntry*>() << pAssetEntry, message, canceled,
		alwaysSaveMessage, alwaysSave, failedToClose);
}


void AssetEntryManager::assetEntryDestroyed(QObject *obj)
{
	IAssetEntry* pAssetEntry = static_cast<IAssetEntry*>(obj);
	// Check the special unwatched first:
	if (!d->assetEntrys_.removeOne(pAssetEntry)) {
	//	removeFileInfo(pAssetEntry);
	}
}


void AssetEntryManager::syncWithEditor(const QList<IContext*>& context)
{
	if (context.isEmpty()) {
		return;
	}

	IEditor *editor = EditorManager::currentEditor();
	if (!editor || editor->assetEntry()->isTemporary()) {
		return;
	}

	foreach(IContext* c, context) 
	{
		if (editor->widget() == c->widget()) {
			setCurrentFile(editor->assetEntry()->name());
			break;
		}
	}
}



bool AssetEntryManager::saveModifiedFilesHelper(const QList<IAssetEntry*>& assetEntrys,
	const QString& message, bool* pCancelled, bool silently,
	const QString& alwaysSaveMessage, bool* pAlwaysSave,
	QList<IAssetEntry*>* pFailedToSave)
{
	X_UNUSED(assetEntrys);
	X_UNUSED(message);
	X_UNUSED(pCancelled);
	X_UNUSED(silently);
	X_UNUSED(alwaysSaveMessage);
	X_UNUSED(pAlwaysSave);
	X_UNUSED(pFailedToSave);


	return false;
}

X_NAMESPACE_END