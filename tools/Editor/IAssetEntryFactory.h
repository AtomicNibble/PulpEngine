#pragma once


#include "id.h"

X_NAMESPACE_BEGIN(assman)

class IAssetEntry;


class IAssetEntryFactory : public QObject
{
	Q_OBJECT

public:
	X_INLINE IAssetEntryFactory(QObject *parent = 0);

	X_INLINE Id id(void) const;
	X_INLINE QString displayName(void) const;

protected:
	X_INLINE void setId(Id id);
	X_INLINE void setDisplayName(const QString &displayName);

private:
	Id id_;
	QString displayName_;
};

X_INLINE IAssetEntryFactory::IAssetEntryFactory(QObject *parent) :
	QObject(parent) 
{
}


X_INLINE Id IAssetEntryFactory::id(void) const
{ 
	return id_;
}

X_INLINE QString IAssetEntryFactory::displayName(void) const 
{ 
	return displayName_; 
}

X_INLINE void IAssetEntryFactory::setId(Id id) 
{
	id_ = id;
}

X_INLINE void IAssetEntryFactory::setDisplayName(const QString& displayName) 
{ 
	displayName_ = displayName; 
}



X_NAMESPACE_END