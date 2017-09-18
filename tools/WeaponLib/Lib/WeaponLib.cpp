#include "stdafx.h"
#include "WeaponLib.h"


X_NAMESPACE_BEGIN(game)


WeaponLib::WeaponLib()
{
}

WeaponLib::~WeaponLib()
{

}

const char* WeaponLib::getOutExtension(void) const
{
	return WEAPON_FILE_EXTENSION;
}

bool WeaponLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
{



	return true;
}


X_NAMESPACE_END