#include "stdafx.h"
#include "AssetEntryModel.h"

#include "IAssetEntry.h"
#include "IEditor.h"

X_NAMESPACE_BEGIN(assman)


AssetEntryModel::Entry::Entry(IAssetEntry* pAssetEntry, QWidget* pEditorWidget,
	const QString& displayName, const QString& assetName, assetDb::AssetType::Enum type, Id id) :
	pAssetEntry_(pAssetEntry),
	pEditorWidget_(pEditorWidget),
	displayName_(displayName),
	assetName_(assetName),
	type_(type),
	id_(id)
{

}

QString AssetEntryModel::Entry::assetName(void) const
{
	return assetName_;
}

QString AssetEntryModel::Entry::displayName(void) const
{
	return displayName_;
}

assetDb::AssetType::Enum AssetEntryModel::Entry::type(void) const
{
	return type_;
}

Id AssetEntryModel::Entry::id(void) const
{
	return id_;
}

// ------------------------------------------------------

AssetEntryModel::AssetEntryModel(QObject *parent) :
	QAbstractItemModel(parent),
	lockedIcon_(QLatin1String(":/core/images/locked.png")),
	unlockedIcon_(QLatin1String(":/core/images/unlocked.png"))
{

}

AssetEntryModel::~AssetEntryModel()
{

}



QIcon AssetEntryModel::lockedIcon(void) const
{
	return lockedIcon_;
}

QIcon AssetEntryModel::unlockedIcon(void) const
{
	return unlockedIcon_;
}


AssetEntryModel::Entry* AssetEntryModel::assetEntryAtRow(int32_t row) const
{
	int32_t entryIndex = row - 1;
	if (entryIndex < 0) {
		return 0;
	}

	return assetEntrys_[entryIndex];
}

int32_t AssetEntryModel::rowOfAssetEntry(IAssetEntry* pAssetEntry) const
{
	if (!pAssetEntry) {
		return 0;
	}
	return indexOfAssetEntry(pAssetEntry) + 1;
}


int32_t AssetEntryModel::assetEntryCount(void) const
{
	return assetEntrys_.count();
}

QList<AssetEntryModel::Entry*> AssetEntryModel::assetEntrys(void) const
{
	return assetEntrys_;
}

int32_t AssetEntryModel::indexOfAssetEntry(IAssetEntry* pAssetEntry) const
{
	for (int32_t i = 0; i < assetEntrys_.count(); ++i) {
		if (assetEntrys_.at(i)->pAssetEntry_ == pAssetEntry) {
			return i;
		}
	}
	return -1;
}

int32_t AssetEntryModel::indexOfAsset(const QString& assetName, assetDb::AssetType::Enum type) const
{
	if (assetName.isEmpty()) {
		return -1;
	}

	for (int32_t i = 0; i < assetEntrys_.count(); ++i) {
		auto pEntry = assetEntrys_.at(i);
		if (pEntry->assetName() == assetName && pEntry->type() == type) {
			return i;
		}
	}

	return -1;
}

AssetEntryModel::Entry* AssetEntryModel::entryForAssetEntry(IAssetEntry* pAssetEntry) const
{
	int32_t index = indexOfAssetEntry(pAssetEntry);
	if (index < 0) {
		return nullptr;
	}
	return assetEntrys_.at(index);
}

QList<IAssetEntry*> AssetEntryModel::openedAssetEntrys(void) const
{
	return editors_.keys();
}

IAssetEntry* AssetEntryModel::assetEntryForAsset(const QString& assetName, assetDb::AssetType::Enum type) const
{
	int32_t index = indexOfAsset(assetName, type);
	if (index < 0) {
		return nullptr;
	}
	return assetEntrys_.at(index)->pAssetEntry_;
}

QList<IEditor*> AssetEntryModel::editorsForAsset(const QString& assetName, assetDb::AssetType::Enum type) const
{
	IAssetEntry* pAssetEntry = assetEntryForAsset(assetName, type);
	if (pAssetEntry) {
		return editorsForAssetEntry(pAssetEntry);
	}
	return QList<IEditor*>();
}

QList<IEditor*> AssetEntryModel::editorsForAssetEntry(IAssetEntry* pAssetEntry) const
{
	return editors_.value(pAssetEntry);
}

QList<IEditor*> AssetEntryModel::editorsForAssetEntrys(const QList<IAssetEntry*>& assetEntrys) const
{
	QList<IEditor*> result;
	for(IAssetEntry* pAssetEntry : assetEntrys) {
		result += editors_.value(pAssetEntry);
	}
	return result;
}



// editor manager related functions, nobody else should call it
void AssetEntryModel::addEditor(IEditor* pEditor, bool* pIsNewDocument)
{
	if (!pEditor) {
		return;
	}

	QList<IEditor*>& editorList = editors_[pEditor->assetEntry()];

	bool isNew = editorList.isEmpty();
	if (pIsNewDocument) {
		*pIsNewDocument = isNew;
	}

	editorList << pEditor;
	if (isNew) {
		IAssetEntry* pAssetEntry = pEditor->assetEntry();

		Entry* pEntry = new Entry(pEditor->assetEntry(), pEditor->widget(), pAssetEntry->displayName(),
			pAssetEntry->name(), pAssetEntry->type(), pEditor->id());
		addEntry(pEntry);
	}
}

void AssetEntryModel::removeEditor(IEditor* pEditor, bool* pLastOneForDocument)
{
	if (pLastOneForDocument) {
		*pLastOneForDocument = false;
	}

	BUG_ASSERT(pEditor, return);

	IAssetEntry* pAssetEntry = pEditor->assetEntry();

	BUG_ASSERT(editors_.contains(pAssetEntry), return);

	editors_[pAssetEntry].removeAll(pEditor);
	if (editors_.value(pAssetEntry).isEmpty())
	{
		if (pLastOneForDocument) {
			*pLastOneForDocument = true;
		}

		editors_.remove(pAssetEntry);
		removeAssetEntry(indexOfAssetEntry(pAssetEntry));
	}
}

void AssetEntryModel::removeAssetEntry(const QString& assetName, assetDb::AssetType::Enum type)
{
	int32_t index = indexOfAsset(assetName, type);

	BUG_ASSERT(!assetEntrys_.at(index)->pAssetEntry_, return); // we wouldn't know what to do with the associated editors

	removeAssetEntry(index);
}

void AssetEntryModel::removeEntry(Entry* pEntry)
{
	BUG_ASSERT(!pEntry->pAssetEntry_, return); // we wouldn't know what to do with the associated editors

	int32_t index = assetEntrys_.indexOf(pEntry);

	removeAssetEntry(index);
}

void AssetEntryModel::addEntry(Entry* pEntry)
{
	QString assetName = pEntry->assetName();
	auto type = pEntry->type();

	// replace a non-loaded entry (aka 'restored') if possible
	int32_t previousIndex = indexOfAsset(assetName, type);
	if (previousIndex >= 0) 
	{
		if (pEntry->pAssetEntry_ && assetEntrys_.at(previousIndex)->pAssetEntry_ == nullptr)
		{
			Entry* previousEntry = assetEntrys_.at(previousIndex);
			assetEntrys_[previousIndex] = pEntry;
			delete previousEntry;
			connect(pEntry->pAssetEntry_, SIGNAL(changed()), this, SLOT(itemChanged()));
		}
		else 
		{
			delete pEntry;
		}
		return;
	}

	int32_t index;
	QString displayName = pEntry->displayName();
	for (index = 0; index < assetEntrys_.count(); ++index) {
		if (displayName < assetEntrys_.at(index)->displayName()) {
			break;
		}
	}

	int32_t row = index + 1;
	beginInsertRows(QModelIndex(), row, row);
	assetEntrys_.insert(index, pEntry);

	if (pEntry->pAssetEntry_) {
		connect(pEntry->pAssetEntry_, SIGNAL(changed()), this, SLOT(itemChanged()));
	}

	endInsertRows();
}


void AssetEntryModel::removeAssetEntry(int32_t idx)
{
	if (idx < 0) {
		return;
	}

	BUG_ASSERT(idx < assetEntrys_.size(), return);

	IAssetEntry* pAssetEntry = assetEntrys_.at(idx)->pAssetEntry_;
	int32_t row = idx + 1;

	beginRemoveRows(QModelIndex(), row, row);
		delete assetEntrys_.takeAt(idx);
	endRemoveRows();

	if (pAssetEntry) {
		disconnect(pAssetEntry, SIGNAL(changed()), this, SLOT(itemChanged()));
	}
}



// QAbstractItemModel
int32_t AssetEntryModel::columnCount(const QModelIndex &parent) const
{
	if (!parent.isValid()) {
		return 2;
	}
	return 0;
}

QVariant AssetEntryModel::data(const QModelIndex &index, int32_t role) const
{
	if (!index.isValid() || (index.column() != 0 && role < Qt::UserRole)) {
		return QVariant();
	}

	int32_t entryIndex = index.row() - 1;

	if (entryIndex < 0)
	{
		switch (role) {
		case Qt::DisplayRole:
			return tr("<no assetEntry>");
		case Qt::ToolTipRole:
			return tr("No AssetEntry is selected.");
		default:
			return QVariant();
		}
	}

	const Entry* pEntry = assetEntrys_.at(entryIndex);
	switch (role) 
	{
	case Qt::DisplayRole:
		return (pEntry->pAssetEntry_ && pEntry->pAssetEntry_->isModified())
			? pEntry->displayName() + QLatin1Char('*')
			: pEntry->displayName();
	case Qt::DecorationRole:
	{
		bool showLock = false;
		if (pEntry->pAssetEntry_) {
			showLock = pEntry->pAssetEntry_->isFileReadOnly();
		}
		else {
			showLock = false;
		}
		return showLock ? lockedIcon_ : QIcon();
	}
	case Qt::ToolTipRole:
		return pEntry->assetName().isEmpty() ? pEntry->displayName()  : QDir::toNativeSeparators(pEntry->assetName());

	default:
		return QVariant();
	}

	return QVariant();
}

QModelIndex AssetEntryModel::parent(const QModelIndex &) const
{
	return QModelIndex();
}

int32_t AssetEntryModel::rowCount(const QModelIndex &parent) const
{
	if (!parent.isValid()) {
		return assetEntrys_.count() + 1;
	}
	return 0;
}

QModelIndex AssetEntryModel::index(int32_t row, int32_t column, const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	if (column < 0 || column > 1 || row < 0 || row >= assetEntrys_.count() + 1) {
		return QModelIndex();
	}
	return createIndex(row, column);
}


void AssetEntryModel::itemChanged(void)
{
	IAssetEntry* pAssetEntry = qobject_cast<IAssetEntry*>(sender());

	int32_t idx = indexOfAssetEntry(pAssetEntry);
	if (idx < 0) {
		return;
	}

	QModelIndex mindex = index(idx + 1, 0);
	emit dataChanged(mindex, mindex);
}


// ~QAbstractItemModel



X_NAMESPACE_END