#include "stdafx.h"
#include "Game.h"
#include "EngineApp.h"

#include <IFileSys.h>

#include <String\Path.h>
#include <Platform\MessageBox.h>

#include <tchar.h>

#define _LAUNCHER

// #undef X_LIB
#include <ModuleExports.h>

#ifdef X_LIB
struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Input")
X_LINK_ENGINE_LIB("Font")
X_LINK_ENGINE_LIB("Core")
X_LINK_ENGINE_LIB("Script")
X_LINK_ENGINE_LIB("Sound")
X_LINK_ENGINE_LIB("RenderDx12")
X_LINK_ENGINE_LIB("3DEngine")
X_LINK_ENGINE_LIB("Physics")
X_LINK_ENGINE_LIB("GameDLL")
X_LINK_ENGINE_LIB("Network")
X_LINK_ENGINE_LIB("Video")

X_FORCE_LINK_FACTORY("XEngineModule_Input")
X_FORCE_LINK_FACTORY("XEngineModule_Font")
X_FORCE_LINK_FACTORY("XEngineModule_Script")
X_FORCE_LINK_FACTORY("XEngineModule_Sound")
X_FORCE_LINK_FACTORY("XEngineModule_3DEngine")
X_FORCE_LINK_FACTORY("XEngineModule_Physics")
X_FORCE_LINK_FACTORY("XEngineModule_Game")
X_FORCE_LINK_FACTORY("XEngineModule_Network")
X_FORCE_LINK_FACTORY("XEngineModule_Video")

X_FORCE_SYMBOL_LINK("?s_factory@XEngineModule_Render@render@Potato@@0V?$XSingletonFactory@VXEngineModule_Render@render@Potato@@@@A");
// X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@render@Potato@@0V1234@A");

// some libs that w link against.
X_LINK_ENGINE_LIB("ImgLib")
X_LINK_ENGINE_LIB("ShaderLib")
X_LINK_ENGINE_LIB("ModelLib")
X_LINK_ENGINE_LIB("AnimLib")
X_LINK_ENGINE_LIB("FontLib")

X_FORCE_LINK_FACTORY("XConverterLib_Img")
X_FORCE_LINK_FACTORY("XConverterLib_Shader")
X_FORCE_LINK_FACTORY("XConverterLib_Model")
X_FORCE_LINK_FACTORY("XConverterLib_Anim")
X_FORCE_LINK_FACTORY("XConverterLib_Font")

#endif // !X_LIB

namespace
{
    core::MallocFreeAllocator gAlloc;

} // namespace

void* operator new(size_t sz)
{
    return gAlloc.allocate(sz, sizeof(uintptr_t), 0);
}

void* operator new[](size_t sz)
{
    return gAlloc.allocate(sz, sizeof(uintptr_t), 0);
}

void operator delete(void* m)
{
    if (m) {
        gAlloc.free(m);
    }
}

void operator delete[](void* m)
{
    if (m) {
        gAlloc.free(m);
    }
}

void operator delete(void* m, size_t sz)
{
    if (m) {
        gAlloc.free(m, sz);
    }
}

void operator delete[](void* m, size_t sz)
{
    if (m) {
        gAlloc.free(m, sz);
    }
}


int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine, int nCmdShow)
{
    X_UNUSED(hPrevInstance);
    X_UNUSED(lpCmdLine);
    X_UNUSED(nCmdShow);

    int nRes = 0;

    { // scope it for leak tests.
        EngineApp engine;

        if (engine.Init(hInstance, lpCmdLine)) {
            nRes = engine.MainLoop();
        }
    }

    return nRes;
}
