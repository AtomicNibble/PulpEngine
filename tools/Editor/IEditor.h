#pragma once


#include <QObject>
#include "Context.h"


X_NAMESPACE_BEGIN(editor)

class IAssetEntry;

class IEditor : public IContext
{
	Q_OBJECT
public:
	IEditor(QObject* pParent = nullptr) : IContext(pParent) {};
	virtual ~IEditor() = default;

	virtual bool open(QString* pErrorString, const QString& assetName, assetDb::AssetType::Enum type) X_ABSTRACT;
	virtual IAssetEntry* assetEntry(void) X_ABSTRACT;
	virtual Id id(void) const X_ABSTRACT;

	X_INLINE virtual bool duplicateSupported(void) const;
	X_INLINE virtual IEditor* duplicate(void);

	X_INLINE virtual QByteArray saveState(void) const;
	X_INLINE virtual bool restoreState(const QByteArray&);

signals:
	void titleChanged(QString);
};


X_NAMESPACE_END

#include "IEditor.inl"