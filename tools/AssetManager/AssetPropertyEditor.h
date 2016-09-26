#pragma once

#include <QObject>
#include "IEditor.h"
#include "IAssetEntry.h"

#include "AssetScriptTypes.h"



X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(assman)

class AssetPropertyEditor;
class AssetPropertyEditorWidget;
class AssetPropsScriptManager;


class AssetProperties : public IAssetEntry
{
	Q_OBJECT

public:
	AssetProperties(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, AssetPropertyEditorWidget* widget);
	virtual ~AssetProperties();


	bool isModified(void) const X_OVERRIDE;
	bool isSaveAsAllowed(void) const X_OVERRIDE;



	const assman::AssetProps& props(void) const;

signals:
	void contentsChanged(void);

private:
	AssetPropertyEditorWidget* getEditor(void);

private:
	assetDb::AssetDB& db_;
	AssetPropsScriptManager* pPropScriptMan_;
	AssetPropertyEditorWidget* pWidget_;
	AssetProps props_;
};


class AssetPropertyEditorWidget : public QScrollArea
{
	Q_OBJECT
public:
	AssetPropertyEditorWidget(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, QWidget *parent = nullptr);
	AssetPropertyEditorWidget(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, AssetProperties* pAssetEntry, QWidget *parent = nullptr);
	AssetPropertyEditorWidget(AssetPropertyEditorWidget* pOther);
	~AssetPropertyEditorWidget();


	bool open(QString* pErrorString, const QString& data, assetDb::AssetType::Enum type);

	AssetPropertyEditor* editor(void);
	AssetProperties* assetProperties(void) const;

protected:
	AssetPropertyEditor* createEditor(void);


private:
	assetDb::AssetDB& db_;
	AssetPropsScriptManager* pPropScriptMan_;
	AssetPropertyEditor* pEditor_;
	QSharedPointer<AssetProperties> assetProps_;
};


class AssetPropertyEditor : public IEditor
{
	Q_OBJECT
public:
	AssetPropertyEditor(AssetPropertyEditorWidget* editorWidget);
	~AssetPropertyEditor();

	bool open(QString* pErrorString, const QString& fileName, assetDb::AssetType::Enum type) X_OVERRIDE;
	IAssetEntry* assetEntry(void) X_OVERRIDE;
	Id id(void) const X_OVERRIDE;

private slots:
	void modificationChanged(bool);

private:
	AssetPropertyEditorWidget* pEditorWidget_;
};



X_NAMESPACE_END