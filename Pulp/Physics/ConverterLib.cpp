#include "stdafx.h"

#include <IPhysics.h>

#include <IConverterModule.h>
#include <Extension\XExtensionMacros.h>



class XConverterLib_Phys : public IConverterModule
{
	// X_POTATO_INTERFACE_SIMPLE(IConverterModule);
	X_POTATO_INTERFACE_BEGIN()             
	X_POTATO_INTERFACE_ADD(IConverterModule)
	X_POTATO_INTERFACE_END()


	X_POTATO_GENERATE_SINGLETONCLASS(XConverterLib_Phys, "Engine_PhysLib",
		0x25f95d5f, 0xef87, 0x4b24, 0x95, 0xd2, 0x2b, 0xc, 0xb0, 0x76, 0xd5, 0x96);


	virtual const char* GetName(void) X_OVERRIDE
	{
		return "Phys";
	}

	virtual IConverter* Initialize(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);


		return nullptr;
	}

	virtual bool ShutDown(IConverter* pCon) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		return true;
	}
};

X_POTATO_REGISTER_CLASS(XConverterLib_Phys);


XConverterLib_Phys::XConverterLib_Phys()
{
}

XConverterLib_Phys::~XConverterLib_Phys()
{
}
