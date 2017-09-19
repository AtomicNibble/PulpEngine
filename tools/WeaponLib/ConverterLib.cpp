#include "stdafx.h"

#include <IConverterModule.h>
#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv

#include "Lib\WeaponLib.h"

namespace
{

	core::MallocFreeAllocator g_WeaponLibAlloc;

} // namespace

WeaponLibArena* g_WeaponLibArena = nullptr;


class XConverterLib_Weapon : public IConverterModule
{
	X_POTATO_INTERFACE_SIMPLE(IConverterModule);

	X_POTATO_GENERATE_SINGLETONCLASS(XConverterLib_Weapon, "Engine_WeaponLib",
		0x33ed9f6, 0xb67, 0x45a6, 0xa7, 0x5c, 0xaa, 0xfb, 0xe, 0xea, 0xf3, 0x3e);

	virtual const char* GetName(void) X_OVERRIDE
	{
		return "Material";
	}

	virtual IConverter* Initialize(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		g_WeaponLibArena = X_NEW(WeaponLibArena, gEnv->pArena, "WeaponLibArena")(&g_WeaponLibAlloc, "WeaponLibArena");

		return X_NEW(game::weapon::WeaponLib, g_WeaponLibArena, "IMaterialLib")();
	}

	virtual bool ShutDown(IConverter* pCon) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		X_DELETE_AND_NULL(pCon, g_WeaponLibArena);
		X_DELETE_AND_NULL(g_WeaponLibArena, gEnv->pArena);
		return true;
	}
};

X_POTATO_REGISTER_CLASS(XConverterLib_Weapon);


XConverterLib_Weapon::XConverterLib_Weapon()
{
}

XConverterLib_Weapon::~XConverterLib_Weapon()
{
}
