#include "stdafx.h"

#include <IConverterModule.h>
#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv

#include "Lib\VideoLib.h"

namespace
{
    core::MallocFreeAllocator g_VideoLibAlloc;

} // namespace

VideoLibArena* g_VideoLibArena = nullptr;

class XConverterLib_Video : public IConverterModule
{
    X_ENGINE_INTERFACE_SIMPLE(IConverterModule);

    X_ENGINE_GENERATE_SINGLETONCLASS(XConverterLib_Video, "Engine_VideoLib",
        0xa4d6c8bb, 0xe822, 0x4e70, 0x8e, 0x1e, 0xf5, 0xe2, 0xa8, 0xb4, 0xda, 0x12);

    virtual const char* GetName(void) X_OVERRIDE
    {
        return "Video";
    }

    virtual IConverter* Initialize(void) X_OVERRIDE
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);

        g_VideoLibArena = X_NEW(VideoLibArena, gEnv->pArena, "VideoLibArena")(&g_VideoLibAlloc, "VideoLibArena");

        return X_NEW(video::VideoLib, g_VideoLibArena, "VideoLib")();
    }

    virtual bool ShutDown(IConverter* pCon) X_OVERRIDE
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);

        X_DELETE_AND_NULL(pCon, g_VideoLibArena);
        X_DELETE_AND_NULL(g_VideoLibArena, gEnv->pArena);
        return true;
    }
};

X_ENGINE_REGISTER_CLASS(XConverterLib_Video);

XConverterLib_Video::XConverterLib_Video()
{
}

XConverterLib_Video::~XConverterLib_Video()
{
}
