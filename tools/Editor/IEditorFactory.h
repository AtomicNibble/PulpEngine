#pragma once

#include "IAssetEntryFactory.h"

X_NAMESPACE_BEGIN(editor)


class IEditor;
class IAssetEntry;

class IEditorFactory : public IAssetEntryFactory
{
	Q_OBJECT

public:
	X_INLINE  IEditorFactory(QObject *parent = 0);

	virtual IEditor* createEditor(void) X_ABSTRACT;
};

X_INLINE IEditorFactory::IEditorFactory(QObject *parent) :
	IAssetEntryFactory(parent)
{

}


X_NAMESPACE_END