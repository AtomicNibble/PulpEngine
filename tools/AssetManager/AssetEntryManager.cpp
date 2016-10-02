#include "stdafx.h"
#include "AssetEntryManager.h"

#include "IAssetEntry.h"
#include "IEditor.h"
#include "EditorManager.h"

#include "SaveItemsDialog.h"

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

		QList<AssetEntryManager::RecentAsset> recentAssets_;

		AssetEntryManager::AssetInfo currentAsset_;
	};


	AssetEntryManagerPrivate::AssetEntryManagerPrivate() 
	{

	}

	static AssetEntryManager* pInstance_ = nullptr;
	static AssetEntryManagerPrivate* d = nullptr;


} // namespace


AssetEntryManager::RecentAsset::RecentAsset(QString name_, assetDb::AssetType::Enum type_, Id id_) :
	name(name_),
	type(type_),
	id(id_)
{

}

bool AssetEntryManager::RecentAsset::operator==(const RecentAsset& oth) const
{
	return id == oth.id && type == oth.type && name == oth.name;
}

// -------------------------------------------------


AssetEntryManager::AssetInfo::AssetInfo(QString name_, assetDb::AssetType::Enum type_) :
	name(name_),
	type(type_)
{

}

bool AssetEntryManager::AssetInfo::operator==(const AssetInfo& oth) const
{
	return type == oth.type && name == oth.name;
}

// -------------------------------------------------


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


// recent files
void AssetEntryManager::addToRecentFiles(const QString& fileName, assetDb::AssetType::Enum type, const Id& editorId)
{
	if (fileName.isEmpty()) {
		return;
	}

	RecentAsset file(fileName, type, editorId);

	QMutableListIterator<RecentAsset> it(d->recentAssets_);
	while (it.hasNext())
	{
		if(file == it.next()) {
			it.remove();
		}
	}

	if (d->recentAssets_.count() > d->MMAX_RECENT_FILES) {
		d->recentAssets_.removeLast();
	}

	d->recentAssets_.prepend(file);
}

void AssetEntryManager::clearRecentFiles(void)
{
	d->recentAssets_.clear();
}

QList<AssetEntryManager::RecentAsset> AssetEntryManager::recentAssets(void)
{
	return d->recentAssets_;
}


void AssetEntryManager::reloadUIforType(assetDb::AssetType::Enum type)
{
	for (IAssetEntry* pAssetEntry : d->assetEntrys_) {
		if (pAssetEntry->type() == type) {
			pAssetEntry->reloadUi();
		}
	}
}

// current file
void AssetEntryManager::setCurrentFile(const QString& name, assetDb::AssetType::Enum type)
{
	AssetInfo asset(name, type);

	if (d->currentAsset_ == asset) {
		return;
	}
	d->currentAsset_ = asset;
	emit pInstance_->currentFileChanged(d->currentAsset_.name, d->currentAsset_.type);
}

AssetEntryManager::AssetInfo AssetEntryManager::currentFile(void)
{
	return d->currentAsset_;
}

bool AssetEntryManager::saveAssetEntry(IAssetEntry* pAssetEntry)
{
	bool res = true;

	QString errorString;
	if (!pAssetEntry->save(errorString)) 
	{
		auto string = errorString.toStdString();
		X_ERROR("AssetMan", "Error saving: \"%s\"", string.c_str());

		QMessageBox::critical(ICore::dialogParent(), tr("Asset Error"),
			tr("Error while saving asset: %1").arg(errorString));

		res = false;
	}

	addAssetEntry(pAssetEntry);
	return res;
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
	if (!d->assetEntrys_.removeOne(pAssetEntry)) {
		// ...
	}
}


void AssetEntryManager::syncWithEditor(const QList<IContext*>& context)
{
	if (context.isEmpty()) {
		return;
	}

	IEditor* editor = EditorManager::currentEditor();
	if (!editor || editor->assetEntry()->isTemporary()) {
		return;
	}

	foreach(IContext* c, context) 
	{
		if (editor->widget() == c->widget()) {
			const auto pAssetEntry = editor->assetEntry();
			setCurrentFile(pAssetEntry->name(), pAssetEntry->type());
			break;
		}
	}
}



bool AssetEntryManager::saveModifiedFilesHelper(const QList<IAssetEntry*>& assetEntrys,
	const QString& message, bool* pCancelled, bool silently,
	const QString& alwaysSaveMessage, bool* pAlwaysSave,
	QList<IAssetEntry*>* pFailedToSave)
{
	if (pCancelled) {
		*pCancelled = false;
	}

	QList<IAssetEntry*> notSaved;
	QMap<IAssetEntry*, QString> modifiedAssetsMap;
	QList<IAssetEntry*> modifiedAssets;

	foreach(IAssetEntry* pEntry, assetEntrys)
	{
		if (pEntry && pEntry->isModified())
		{
			QString name = pEntry->name();

			// There can be several IDocuments pointing to the same file
			if (!modifiedAssetsMap.key(name, 0) || !pEntry->isFileReadOnly()) {
				modifiedAssetsMap.insert(pEntry, name);
			}
		}
	}

	modifiedAssets = modifiedAssetsMap.keys();
	if (!modifiedAssets.isEmpty())
	{
		QList<IAssetEntry*> assetsToSave;
		if (silently)
		{
			assetsToSave = modifiedAssets;
		}
		else 
		{
			SaveItemsDialog dia(ICore::dialogParent(), modifiedAssets);

			if (!message.isEmpty()) {
				dia.setMessage(message);
			}
			if (!alwaysSaveMessage.isNull()) {
				dia.setAlwaysSaveMessage(alwaysSaveMessage);
			}
			if (dia.exec() != QDialog::Accepted) {
				if (pCancelled) {
					*pCancelled = true;
				}
				if (pAlwaysSave) {
					*pAlwaysSave = dia.alwaysSaveChecked();
				}
				if (pFailedToSave) {
					*pFailedToSave = modifiedAssets;
				}
				return false;
			}
			if (pAlwaysSave) {
				*pAlwaysSave = dia.alwaysSaveChecked();
			}

			assetsToSave = dia.itemsToSave();
		}


		foreach(IAssetEntry* pEntry, assetsToSave)
		{
			if (!EditorManager::saveAssetEntry(pEntry))
			{
				if (pCancelled) {
					*pCancelled = true;
				}
				notSaved.append(pEntry);
			}
		}
	}

	if (pFailedToSave) {
		*pFailedToSave = notSaved;
	}

	return true;
}

X_NAMESPACE_END