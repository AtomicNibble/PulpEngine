#pragma once

#include <QObject>
#include "IEditor.h"


X_NAMESPACE_BEGIN(assman)

class AssetPropertEditor;
class AssetPropertEditorWidget : public QScrollArea
{
	Q_OBJECT
public:
	AssetPropertEditorWidget(QWidget *parent = nullptr);
	~AssetPropertEditorWidget();


	bool open(QString* pErrorString, const QString& fileName);

	AssetPropertEditor* editor(void);

protected:
	AssetPropertEditor* createEditor(void);


private:
	AssetPropertEditor* pEditor_;
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