#pragma once

#include <QObject>
#include "IEditorFactory.h"

X_NAMESPACE_BEGIN(assman)

class AssetPropertyEditorFactory : public IEditorFactory
{
	Q_OBJECT

public:
	AssetPropertyEditorFactory(QObject *parent);

	IEditor* createEditor(void);
};


X_NAMESPACE_END