#pragma once

#include <EngineCommon.h>

#define IPRENDER_EXPORTS

#include <IRender.h>


X_DISABLE_WARNING(4005)


#include <../../3rdparty/source/directx/D3D12.h>
#include <../../3rdparty/source/directx/D3D11.h>
#include <../../3rdparty/source/directx/d3dx10.h>

#include <dxgi1_4.h>

X_ENABLE_WARNING(4005)

#include "Constants.h"
#include "DxTypeHelpers.h"


X_LINK_LIB("D3D12.lib")
X_LINK_LIB("dxgi.lib")
X_LINK_LIB("dxguid")
X_LINK_LIB("d3dcompiler.lib")


// Img Lib
#include <../../tools/ImgLib/ImgLib.h>

X_LINK_LIB("engine_ImgLib");

X_NAMESPACE_BEGIN(render)

extern core::MemoryArenaBase* g_rendererArena;
extern core::MemoryArenaBase* g_textureDataArena;

X_NAMESPACE_END