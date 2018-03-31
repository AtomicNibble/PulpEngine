#pragma once

#include <QObject>
#include "IEditorFactory.h"


X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(assman)

class FxEditorActionHandler;

class FxEditorFactory : public IEditorFactory
{
	Q_OBJECT

public:
	FxEditorFactory(assetDb::AssetDB& db, QObject *parent);
	~FxEditorFactory();

	IEditor* createEditor(void);

private:
	assetDb::AssetDB& db_;
	FxEditorActionHandler* pActionHandler_;
};


X_NAMESPACE_END