#include "stdafx.h"

#include <IConverterModule.h>
#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv

#include "Lib\MaterialLib.h"

namespace
{
    core::MallocFreeAllocator g_MatLibAlloc;

} // namespace

MatLibArena* g_MatLibArena = nullptr;

class XConverterLib_Material : public IConverterModule
{
    X_ENGINE_INTERFACE_SIMPLE(IConverterModule);

    X_ENGINE_GENERATE_SINGLETONCLASS(XConverterLib_Material, "Engine_MaterialLib",
        0x6d9a2569, 0xb55f, 0x4c76, 0x90, 0x85, 0x8e, 0x4e, 0x16, 0xac, 0xae, 0xeb);

    virtual const char* GetName(void) X_OVERRIDE
    {
        return "Material";
    }

    virtual IConverter* Initialize(void) X_OVERRIDE
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);

        g_MatLibArena = X_NEW(MatLibArena, gEnv->pArena, "MaterialLibArena")(&g_MatLibAlloc, "MaterialLibArena");

        return X_NEW(engine::MaterialLib, g_MatLibArena, "IMaterialLib")();
    }

    virtual bool ShutDown(IConverter* pCon) X_OVERRIDE
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);

        X_DELETE_AND_NULL(pCon, g_MatLibArena);
        X_DELETE_AND_NULL(g_MatLibArena, gEnv->pArena);
        return true;
    }
};

X_ENGINE_REGISTER_CLASS(XConverterLib_Material);

XConverterLib_Material::XConverterLib_Material()
{
}

XConverterLib_Material::~XConverterLib_Material()
{
}
