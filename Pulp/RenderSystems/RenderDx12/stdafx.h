#pragma once

#include <EngineCommon.h>

#include <Containers\Fifo.h>
#include <Containers\Array.h>


#define IPRENDER_EXPORTS

#include <IRender.h>

X_DISABLE_WARNING(4005)

#include <../../3rdparty/source/directx/D3D12.h>
#include <../../3rdparty/source/directx/D3D11.h>
#include <../../3rdparty/source/directx/d3dx10.h>

#include <dxgi1_4.h>
#include <wrl/client.h>

X_ENABLE_WARNING(4005)

#include "Util/Config.h"
#include "Util/ErrorStr.h"
#include "Util/Types.h"
#include "Util/Constants.h"
#include "Util/DxTypeHelpers.h"
#include "Util/DebugNameUtil.h"


X_LINK_LIB("D3D12.lib")
X_LINK_LIB("dxgi.lib")
X_LINK_LIB("dxguid")
X_LINK_LIB("d3dcompiler.lib")


// pickle lib.
#include <../../tools/ShaderLib/ShaderLib.h>


X_LINK_ENGINE_LIB("ImgLib");
X_LINK_ENGINE_LIB("ShaderLib");

X_NAMESPACE_BEGIN(render)

extern core::MemoryArenaBase* g_rendererArena;
extern core::MemoryArenaBase* g_textureDataArena;

X_NAMESPACE_END


/*

Some notes about dx12, mostly about what's thread safe / thread free
as the info is spread all over the docs.

notes:

Threading:
	* All methods for creating descriptors are free threaded.

	* The ID3D12Device::CopyDescriptors and ID3D12Device::CopyDescriptorsSimple methods on the device interface
	use the CPU to immediately copy descriptors. They can be called free threaded as long as multiple threads 
	on the CPU or GPU do not perform any potentially conflicting writes.


	* Create PSOs on worker threads asynchronously
	PSO creation is where shaders compilation and related stalls happen



Descriptor heap useful: https://msdn.microsoft.com/en-us/library/windows/desktop/mt709128(v=vs.85).aspx


links: https://developer.nvidia.com/dx12-dos-and-donts

*/