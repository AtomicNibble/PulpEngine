#pragma once

#include <QObject>
#include "IEditor.h"
#include "IAssetEntry.h"

X_NAMESPACE_BEGIN(assman)

class AssetPropertEditor;
class AssetPropertEditorWidget;


class AssetProperties : public IAssetEntry
{
public:
	AssetProperties(AssetPropertEditorWidget* widget);
	virtual ~AssetProperties();


	bool isModified(void) const X_OVERRIDE;
	bool isSaveAsAllowed(void) const X_OVERRIDE;


private:


};


class AssetPropertEditorWidget : public QScrollArea
{
	Q_OBJECT
public:
	AssetPropertEditorWidget(QWidget *parent = nullptr);
	AssetPropertEditorWidget(AssetProperties* pAssetEntry, QWidget *parent = nullptr);
	AssetPropertEditorWidget(AssetPropertEditorWidget* pOther);
	~AssetPropertEditorWidget();


	bool open(QString* pErrorString, const QString& fileName);

	AssetPropertEditor* editor(void);
	AssetProperties* assetProperties(void) const;

protected:
	AssetPropertEditor* createEditor(void);


private:
	AssetPropertEditor* pEditor_;
	QSharedPointer<AssetProperties> assetProps_;
};


class AssetPropertEditor : public IEditor
{
	Q_OBJECT
public:
	AssetPropertEditor(AssetPropertEditorWidget* editorWidget);
	~AssetPropertEditor();

	bool open(QString* pErrorString, const QString& fileName) X_OVERRIDE;
	IAssetEntry* assetEntry(void) X_OVERRIDE;
	Id id(void) const X_OVERRIDE;

private:
	AssetPropertEditorWidget* pEditorWidget_;
};



X_NAMESPACE_END