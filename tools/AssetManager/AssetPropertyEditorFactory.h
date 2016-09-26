#pragma once

#include <QObject>
#include "IEditorFactory.h"


X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(assman)

class AssetPropsScriptManager;

class AssetPropertyEditorFactory : public IEditorFactory
{
	Q_OBJECT

public:
	AssetPropertyEditorFactory(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, QObject *parent);

	IEditor* createEditor(void);

private:
	assetDb::AssetDB& db_;
	AssetPropsScriptManager* pPropScriptMan_;
};


X_NAMESPACE_END