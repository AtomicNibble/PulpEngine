#include "stdafx.h"
#include "IAssetEntry.h"

X_NAMESPACE_BEGIN(assman)


IAssetEntry::IAssetEntry(QObject *parent) :
	QObject(parent),
	temporary_(false)
{

}

IAssetEntry::~IAssetEntry()
{

}



QString IAssetEntry::name(void) const
{
	return displayName_;
}

QString IAssetEntry::displayName(void) const
{
	return displayName_;
}


void IAssetEntry::setDisplayName(const QString& name)
{
	displayName_ = name;
}

assetDb::AssetType::Enum IAssetEntry::type(void) const
{
	return type_;
}

void IAssetEntry::setType(assetDb::AssetType::Enum type)
{
	type_ = type;
}


bool IAssetEntry::isFileReadOnly(void) const
{
	return false;
}

bool IAssetEntry::isTemporary(void) const
{
	return temporary_;

}

void IAssetEntry::setTemporary(bool temporary)
{
	temporary_ = temporary;
}


bool IAssetEntry::shouldAutoSave(void) const
{
	return true;
}


bool IAssetEntry::autoSave(QString* pErrorString, const QString& fileName)
{
	X_ASSERT_NOT_IMPLEMENTED();
	X_UNUSED(pErrorString);
	X_UNUSED(fileName);

	return false;
}




X_NAMESPACE_END