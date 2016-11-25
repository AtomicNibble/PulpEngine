
#pragma once


#include <EngineCommon.h>



typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
#if X_DEBUG
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	core::SimpleMemoryTagging
#else
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
#endif // !X_DEBUG
> ShaderLibArena;

extern ShaderLibArena* g_ShaderLibArena;


// this shader lib is dx12 specific for now.
// but even so very little of the logic is actuall dx12 specific.
// just no point splitting things out yet.
X_DISABLE_WARNING(4005)

#include <../../3rdparty/source/directx/D3D12.h>
#include <../../3rdparty/source/directx/D3D11.h>
#include <../../3rdparty/source/directx/d3dx10.h>

#include <D3Dcompiler.h>
#include <../../3rdparty/source/directx/D3DX9Shader.h>

X_ENABLE_WARNING(4005)

X_LINK_LIB("dxguid")
X_LINK_LIB("d3dcompiler.lib")
