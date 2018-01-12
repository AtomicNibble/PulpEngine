#pragma once


#include <EngineCommon.h>

#define IPRENDER_EXPORTS

#include <IRender.h>

X_DISABLE_WARNING(4005)

// #include <windowsx.h>
// #include <D3D11.h>
// #include <d3dx10.h>

#if X_DEBUG // needed for the debug api.
#include <../../3rdparty/source/directx/D3D11.h>
#include <../../3rdparty/source/directx/D3Dx11.h>
#endif

#include <../../3rdparty/source/directx/D3D11.h>
#include <../../3rdparty/source/directx/d3dx10.h>

X_ENABLE_WARNING(4005)


#include <IFont.h>
#include <Math\MatrixStack.h>

#include "DX10Util.h"


// Img Lib
#include <../../tools/ImgLib/ImgLib.h>

X_LINK_ENGINE_LIB("ImgLib");

extern core::MemoryArenaBase* g_rendererArena;
extern core::MemoryArenaBase* g_textureDataArena;




