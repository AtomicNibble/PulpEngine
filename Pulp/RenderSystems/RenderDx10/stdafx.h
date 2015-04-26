#pragma once


#include <EngineCommon.h>

#define IPRENDER_EXPORTS

#include <IRender.h>

X_DISABLE_WARNING(4005)

// #include <windowsx.h>
// #include <D3D11.h>
// #include <d3dx10.h>

#if X_DEBUG // needed for the debug api.
 #include <D3D11.h>
 #include <D3Dx11.h>
#endif

#include <D3D11.h>
#include <d3dx10.h>

X_ENABLE_WARNING(4005)


#include <IFont.h>

#include "DX10Util.h"
#include "../Common/MatrixStack.h"




extern core::MemoryArenaBase* g_rendererArena;
extern core::MemoryArenaBase* g_textureArena;




