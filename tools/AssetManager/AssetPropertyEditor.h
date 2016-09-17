#pragma once

#include <QObject>
#include "IEditor.h"
#include "IAssetEntry.h"

X_NAMESPACE_BEGIN(assman)

class AssetPropertyEditor;
class AssetPropertyEditorWidget;


class AssetProperties : public IAssetEntry
{
public:
	AssetProperties(AssetPropertyEditorWidget* widget);
	virtual ~AssetProperties();


	bool isModified(void) const X_OVERRIDE;
	bool isSaveAsAllowed(void) const X_OVERRIDE;


private:


};


class AssetPropertyEditorWidget : public QScrollArea
{
	Q_OBJECT
public:
	AssetPropertyEditorWidget(QWidget *parent = nullptr);
	AssetPropertyEditorWidget(AssetProperties* pAssetEntry, QWidget *parent = nullptr);
	AssetPropertyEditorWidget(AssetPropertyEditorWidget* pOther);
	~AssetPropertyEditorWidget();


	bool open(QString* pErrorString, const QString& fileName);

	AssetPropertyEditor* editor(void);
	AssetProperties* assetProperties(void) const;

protected:
	AssetPropertyEditor* createEditor(void);


private:
	AssetPropertyEditor* pEditor_;
	QSharedPointer<AssetProperties> assetProps_;
};


class AssetPropertyEditor : public IEditor
{
	Q_OBJECT
public:
	AssetPropertyEditor(AssetPropertyEditorWidget* editorWidget);
	~AssetPropertyEditor();

	bool open(QString* pErrorString, const QString& fileName) X_OVERRIDE;
	IAssetEntry* assetEntry(void) X_OVERRIDE;
	Id id(void) const X_OVERRIDE;

private:
	AssetPropertyEditorWidget* pEditorWidget_;
};



X_NAMESPACE_END