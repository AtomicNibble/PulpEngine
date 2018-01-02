#pragma once

#include <QObject>
#include "IEditorFactory.h"


X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(editor)

class AssetPropsScriptManager;
class AssetPropsEditorActionHandler;

class AssetPropertyEditorFactory : public IEditorFactory
{
	Q_OBJECT

public:
	AssetPropertyEditorFactory(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, QObject *parent);
	~AssetPropertyEditorFactory();

	IEditor* createEditor(void);

private:
	assetDb::AssetDB& db_;
	AssetPropsScriptManager* pPropScriptMan_;
	AssetPropsEditorActionHandler* pActionHandler_;
};


X_NAMESPACE_END