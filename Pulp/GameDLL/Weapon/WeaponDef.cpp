#include "stdafx.h"
#include "WeaponDef.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{
	WeaponDef::WeaponDef(core::string& name) :
		AssetBase(name)
	{

	}

	bool WeaponDef::processData(core::XFile* pFile)
	{
		if (pFile->readObj(hdr_) != sizeof(hdr_)) {
			return false;
		}

		if (!hdr_.isValid()) {
			X_ERROR("WeaponDef", "Header is invalid: \"%s\"", getName().c_str());
			return false;
		}


		return true;
	}


} // namespace weapon

X_NAMESPACE_END