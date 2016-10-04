#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)

class IAssetEntry;
class IEditor;


class AssetEntryModel : public QAbstractItemModel
{
	Q_OBJECT

public:

	struct Entry 
	{
		Entry(IAssetEntry* pAssetEntry, QWidget* pEditorWidget,
			const QString& displayName, const QString& assetName, assetDb::AssetType::Enum type, Id id);

		QString assetName(void) const;
		QString displayName(void) const;
		assetDb::AssetType::Enum type(void) const;
		Id id(void) const;

	public:
		IAssetEntry* pAssetEntry_;
		QWidget* pEditorWidget_;
		QString displayName_;
		QString assetName_;
		assetDb::AssetType::Enum type_;
		Id id_;
	};

public:
	AssetEntryModel(QObject *parent);
	~AssetEntryModel();


	QIcon lockedIcon(void) const;
	QIcon unlockedIcon(void) const;


	Entry* assetEntryAtRow(int32_t row) const;
	int32_t rowOfAssetEntry(IAssetEntry* pAssetEntry) const;

	int32_t assetEntryCount(void) const;
	QList<Entry*> assetEntrys(void) const;
	int32_t indexOfAssetEntry(IAssetEntry* pAssetEntry) const;
	int32_t indexOfAsset(const QString& assetName, assetDb::AssetType::Enum type) const;
	Entry* entryForAssetEntry(IAssetEntry* pAssetEntry) const;
	QList<IAssetEntry*> openedAssetEntrys(void) const;

	IAssetEntry* assetEntryForAsset(const QString& assetName, assetDb::AssetType::Enum type) const;
	QList<IEditor*> editorsForAsset(const QString& assetName, assetDb::AssetType::Enum type) const;
	QList<IEditor*> editorsForAssetEntry(IAssetEntry* pAssetEntry) const;
	QList<IEditor*> editorsForAssetEntrys(const QList<IAssetEntry*>& assetEntrys) const;

	// editor manager related functions, nobody else should call it
	void addEditor(IEditor* pEditor, bool* pIsNewDocument);
	void removeEditor(IEditor* pEditor, bool* pLastOneForDocument);
	void removeAssetEntry(const QString& assetName, assetDb::AssetType::Enum type);
	void removeEntry(Entry* pEntry);


	// QAbstractItemModel
	int32_t columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int32_t role = Qt::DisplayRole) const;
	QModelIndex parent(const QModelIndex &) const;
	int32_t rowCount(const QModelIndex &parent = QModelIndex()) const;
	QModelIndex index(int row, int32_t column = 0, const QModelIndex &parent = QModelIndex()) const;
	// ~QAbstractItemModel


private slots:
	void itemChanged(void);

private:
	void addEntry(Entry* pEntry);
	void removeAssetEntry(int32_t idx);

private:

	const QIcon lockedIcon_;
	const QIcon unlockedIcon_;

	QList<Entry*> assetEntrys_;
	QMap<IAssetEntry*, QList<IEditor*> > editors_;
};


X_NAMESPACE_END
