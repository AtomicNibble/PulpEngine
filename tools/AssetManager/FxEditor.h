#pragma once

#include <QObject>
#include "IEditor.h"
#include "IAssetEntry.h"


X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(assman)

class FxEditor;
class FxEditorWidget;

class AssetFxWidget;

class FxProperties : public IAssetEntry
{
	Q_OBJECT

public:
	FxProperties(assetDb::AssetDB& db, FxEditorWidget* widget);
	virtual ~FxProperties();

	void setWidget(QWidget* widget);

	// IAssetEntry
	bool save(QString& errorString) X_OVERRIDE;
	bool updateRawFile(const ByteArr& compressedData) X_OVERRIDE;
	bool getRawFile(ByteArr& rawData) X_OVERRIDE;
	bool updateThumb(const ByteArr& data, Vec2i thumbDim, Vec2i srcDim,
		core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl) X_OVERRIDE;
	bool getThumb(ByteArr& data, Vec2i& dim) X_OVERRIDE;

	bool reloadUi(void) X_OVERRIDE;

	bool isModified(void) const X_OVERRIDE;
	bool isSaveAsAllowed(void) const X_OVERRIDE;
	// ~IAssetEntry

	bool loadProps(QString& errorString, const QString& fileName, assetDb::AssetType::Enum type);

private:
	void setNotModifiedModified(void);
	FxEditorWidget* getEditor(void);

signals:
	void modificationChanged(bool);

private slots:
	void valueChanged(void);

private:
	assetDb::AssetDB& db_;
	FxEditorWidget* pWidget_;
	AssetFxWidget* pEditorWidget_;

private:
	core::string saved_;
	core::string current_;
};


class FxEditorWidget : public QScrollArea
{
	Q_OBJECT
public:
	FxEditorWidget(assetDb::AssetDB& db, QWidget *parent = nullptr);
	FxEditorWidget(assetDb::AssetDB& db, FxProperties* pAssetEntry, QWidget *parent = nullptr);

	FxEditorWidget(FxEditorWidget* pOther);
	~FxEditorWidget();

private:
	void ctor(const QSharedPointer<FxProperties>& props);

public:
	bool open(QString* pErrorString, const QString& data, assetDb::AssetType::Enum type);

	bool isUndoAvailable(void) const;
	bool isRedoAvailable(void) const;
	bool isReadOnly(void) const;

	void undo(void);
	void redo(void);
	void copy(void);
	void cut(void);
	void paste(void);

	FxEditor* editor(void);
	FxProperties* fxProperties(void) const;

protected:
	FxEditor* createEditor(void);

	void contextMenuEvent(QContextMenuEvent* e);
	void showDefaultContextMenu(QContextMenuEvent* e, const Id menuContextId);

	void appendMenuActionsFromContext(QMenu* menu, const Id menuContextId);
	void appendStandardContextMenuActions(QMenu* menu);

signals:
	void undoAvailable(bool);
	void readOnlyChanged(void);
	void modificationChanged(bool);
	void copyAvailable(bool);

private:
	assetDb::AssetDB& db_;
	FxEditor* pEditor_;
	QSharedPointer<FxProperties> fxProps_;
};

class FxEditor : public IEditor
{
	Q_OBJECT
public:
	FxEditor(FxEditorWidget* editorWidget);
	~FxEditor();

	bool open(QString* pErrorString, const QString& fileName, assetDb::AssetType::Enum type) X_OVERRIDE;
	IAssetEntry* assetEntry(void) X_OVERRIDE;
	Id id(void) const X_OVERRIDE;

	bool duplicateSupported(void) const X_OVERRIDE;
	IEditor* duplicate(void) X_OVERRIDE;

private slots:
	void modificationChanged(bool);

private:
	FxEditorWidget* pEditorWidget_;
};



X_NAMESPACE_END