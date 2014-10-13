#include "stdafx.h"
#include "VidMemManager.h"

#include <Memory\VirtualMem.h>

#include "../Dx10Render.h"

X_NAMESPACE_BEGIN(render)

namespace
{
	static const size_t LOOK_UP_STARTING_SIZE = 2048;
	static const size_t FREE_ID_SIZE = 128;
	static const size_t MAX_BUFFERS = 2048;

	static D3D11_MAP D3DMapType(MapType::Enum type)
	{
#if X_DEBUG
		D3D11_MAP d3dtype;
		switch (type)
		{
			case MapType::READ:
				d3dtype = D3D11_MAP::D3D11_MAP_READ;
				break;
			case MapType::WRITE:
				d3dtype = D3D11_MAP::D3D11_MAP_WRITE;
				break;
			case MapType::WRITE_DISCARD:
				d3dtype = D3D11_MAP::D3D11_MAP_WRITE_DISCARD;
				break;
			case MapType::READ_WRITE:
				d3dtype = D3D11_MAP::D3D11_MAP_READ_WRITE;
				break;
			case MapType::WRITE_NO_OVERWRITE:
				d3dtype = D3D11_MAP::D3D11_MAP_WRITE_NO_OVERWRITE;
				break;
			default:
				d3dtype = D3D11_MAP::D3D11_MAP_READ;
				X_ASSERT_UNREACHABLE();
			break;
		}

		X_ASSERT((D3D11_MAP)(type) == d3dtype, "maptype did not map to the correct d3d enum value")(d3dtype, type, MapType::ToString(type));
		return d3dtype;
#else
		return (D3D11_MAP)type;
#endif // !X_DEBUG
	}
}

VidMemManager::Stats::Stats() 
{
	core::zero_this(this);
}


VidMemManager::VidMemManager() :
	heap_(
		core::bitUtil::RoundUpToMultiple<size_t>(MAX_BUFFERS * sizeof(X3DBuffer), 
		core::VirtualMem::GetPageSize())
		),
	pool_(heap_.start(), heap_.end(), sizeof(X3DBuffer), sizeof(X3DBuffer*), 0 ),
	arena_(&pool_, "VidMemBuffer"),
	
	VBs_(nullptr),
	IBs_(nullptr),
	idLookup_(nullptr),
	freeIds_(nullptr)
{


}

VidMemManager::~VidMemManager()
{


}

// --------------------------------------------------------

void VidMemManager::StartUp(void)
{
	VBs_.setArena(g_rendererArena);
	IBs_.setArena(g_rendererArena);
	idLookup_.setArena(g_rendererArena);
	freeIds_.setArena(g_rendererArena);

	idLookup_.reserve(LOOK_UP_STARTING_SIZE);
	freeIds_.reserve(FREE_ID_SIZE);
	
	g_rendererArena->addChildArena(&arena_);
}

void VidMemManager::ShutDown(void)
{
	X_LOG0("VidMemMng", "Shutting Down");

	// ok shit face
	freeIds_.free();
	IBs_.free();
	idLookup_.free();
	freeIds_.free();


}

// --------------------------------------------------------


uint32_t VidMemManager::CreateIB(uint32_t size, CpuAccessFlags flags)
{
	return CreateIB(size, nullptr, flags);
}

uint32_t VidMemManager::CreateVB(uint32_t size, CpuAccessFlags flags)
{
	return CreateVB(size, nullptr, flags);
}

uint32_t VidMemManager::CreateIB(uint32_t size, const void* pData, CpuAccessFlags flags)
{
	X3DBuffer* pBuf = Int_CreateIB(size);

	DeviceCreateIB(pBuf, pData, flags);

	return createIdForBuffer(pBuf);
}

uint32_t VidMemManager::CreateVB(uint32_t size, const void* pData, CpuAccessFlags flags)
{
	X3DBuffer* pBuf = Int_CreateVB(size);

	DeviceCreateVB(pBuf, pData, flags);

	return createIdForBuffer(pBuf);
}


// --------------------------------------------------------


void VidMemManager::freeIB(uint32_t IBid)
{
	if (IBid == null_id) // we allow null_id on 'free' methords
		return;

	X3DBuffer* pBuf = IBFromId(IBid);

	Int_freeIB(pBuf);
}

void VidMemManager::freeVB(uint32_t VBid)
{
	if (VBid == null_id) // we allow null_id on 'free' methords
		return;

	X3DBuffer* pBuf = VBFromId(VBid);

	Int_freeVB(pBuf);
}


// --------------------------------------------------------


X3DBuffer* VidMemManager::IBFromId(uint32_t bufID) const
{
	X3DBuffer* pBuf = bufferForId(bufID);

	X_ASSERT(pBuf->getBufferType() == VidBufType::INDEX, "buffer id is not a IndexBuf")(bufID);

	return pBuf;
}

X3DBuffer* VidMemManager::VBFromId(uint32_t bufID) const
{
	X3DBuffer* pBuf = bufferForId(bufID);

	X_ASSERT(pBuf->getBufferType() == VidBufType::VERTEX, "buffer id is not a VertexBuf")(bufID);

	return pBuf;
}

// --------------------------------------------------------

void* VidMemManager::MapIB(uint32_t IBid, MapType::Enum maptype)
{
	X3DBuffer* pBuf = IBFromId(IBid);
	X_ASSERT_NOT_NULL(pBuf); // not gonna allow calling this on a invalid ID.

	if (pBuf->isLocked())
	{
		X_WARNING("VidMem", "index buffer is already locked: %i", IBid);
		return pBuf->pLockedData;
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	g_Dx11D3D.DxDeviceContext()->Map(pBuf->pDevBuf, 0, D3DMapType(maptype), 0, &mappedResource);

	pBuf->locked_ = true;
	pBuf->pLockedData = mappedResource.pData;

	return pBuf->pLockedData;
}

void* VidMemManager::MapVB(uint32_t VBid, MapType::Enum maptype)
{
	X3DBuffer* pBuf = VBFromId(VBid);
	X_ASSERT_NOT_NULL(pBuf);

	if (pBuf->isLocked())
	{
		X_WARNING("VidMem", "vertex buffer is already locked: %i", VBid);
		return pBuf->pLockedData;
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	g_Dx11D3D.DxDeviceContext()->Map(pBuf->pDevBuf, 0, D3DMapType(maptype), 0, &mappedResource);

	pBuf->locked_ = true;
	pBuf->pLockedData = mappedResource.pData;

	return pBuf->pLockedData;
}

void VidMemManager::UnMapIB(uint32_t IBid)
{
	X3DBuffer* pBuf = IBFromId(IBid);
	X_ASSERT_NOT_NULL(pBuf);

	if (!pBuf->isLocked())
	{
		X_WARNING("VidMem", "index buffer is not locked: %i", IBid);
		return;
	}

	pBuf->locked_ = false;
	pBuf->pLockedData = nullptr;

	g_Dx11D3D.DxDeviceContext()->Unmap(pBuf->pDevBuf, 0);
}

void VidMemManager::UnMapVB(uint32_t VBid)
{
	X3DBuffer* pBuf = VBFromId(VBid);
	X_ASSERT_NOT_NULL(pBuf);

	if (!pBuf->isLocked())
	{
		X_WARNING("VidMem", "vertex buffer is not locked: %i", VBid);
		return;
	}

	pBuf->locked_ = false;
	pBuf->pLockedData = nullptr;

	g_Dx11D3D.DxDeviceContext()->Unmap(pBuf->pDevBuf, 0);
	
}

// --------------------------------------------------------

uint32_t VidMemManager::createIdForBuffer(X3DBuffer* pBuf)
{
	if (!freeIds_.isEmpty()) {
		uint32_t id = freeIds_.peek();

		freeIds_.pop();
		idLookup_[id] = pBuf;

		return id;
	}
	return safe_static_cast<uint32_t,size_t>(idLookup_.append(pBuf));
}


X3DBuffer* VidMemManager::bufferForId(uint32_t bufID) const
{
	// will assert on invalid index.
	X_ASSERT(bufID != null_id,"bufid can't be a nullid")(bufID, null_id);
	return idLookup_[bufID];
}


void VidMemManager::FreeId(uint32_t id)
{
	idLookup_[id] = nullptr;
	freeIds_.push(id);
}

// --------------------------------------------------------


X3DBuffer* VidMemManager::Int_CreateIB(uint32_t size)
{
	X3DBuffer* pBuf = X_NEW_ALIGNED(X3DBuffer, &arena_, "VidMemIndexs", sizeof(X3DBuffer*));
	pBuf->type = VidBufType::INDEX;
	pBuf->sizeBytes = size;

#if X_ENABLE_VID_MEMORY_STATS
	stats_.indexesBytes += size;
	stats_.maxIndexesBytes = core::Max(stats_.maxIndexesBytes, stats_.indexesBytes);
	stats_.numIndexBuffers++;
	stats_.maxIndexBuffers = core::Max(stats_.maxIndexBuffers, stats_.numIndexBuffers);
#endif // !X_ENABLE_VID_MEMORY_STATS
	return pBuf;
}

X3DBuffer* VidMemManager::Int_CreateVB(uint32_t size)
{
	X3DBuffer* pBuf = X_NEW_ALIGNED(X3DBuffer, &arena_, "VidMemVertex", sizeof(X3DBuffer*));
	pBuf->type = VidBufType::VERTEX;
	pBuf->sizeBytes = size;

#if X_ENABLE_VID_MEMORY_STATS
	stats_.vertexBytes += size;
	stats_.maxVertexBytes = core::Max(stats_.maxVertexBytes, stats_.vertexBytes);
	stats_.numVertexBuffers++;
	stats_.maxVertexBuffers = core::Max(stats_.maxVertexBuffers, stats_.numVertexBuffers);
#endif // !X_ENABLE_VID_MEMORY_STATS
	return pBuf;
}


// ---------------------------------------------------------------


void VidMemManager::Int_freeIB(X3DBuffer* pIB)
{
	X_ASSERT_NOT_NULL(pIB);
	X_ASSERT(pIB->getBufferType() == VidBufType::INDEX, "invalid buffer type")();


	DeviceUnsetIndexs(pIB);

	core::SafeReleaseDX(pIB->pDevBuf);


#if X_ENABLE_VID_MEMORY_STATS
	stats_.indexesBytes -= pIB->getSizeInBytes();
	stats_.numIndexBuffers--;
#endif // !X_ENABLE_VID_MEMORY_STATS

	X_DELETE(pIB, &arena_);
}

void VidMemManager::Int_freeVB(X3DBuffer* pVB)
{
	X_ASSERT_NOT_NULL(pVB);
	X_ASSERT(pVB->getBufferType() == VidBufType::VERTEX, "invalid buffer type")();

	DeviceUnsetIndexs(pVB);

	core::SafeReleaseDX(pVB->pDevBuf);

#if X_ENABLE_VID_MEMORY_STATS
	stats_.vertexBytes -= pVB->getSizeInBytes();
	stats_.numVertexBuffers--;
#endif // !X_ENABLE_VID_MEMORY_STATS

	X_DELETE(pVB, &arena_);
}

// ---------------------------------------------------------------

bool VidMemManager::DeviceCreateVB(X3DBuffer* pBuf, const void* pData, const CpuAccessFlags flags)
{
	X_ASSERT(pBuf->getSizeInBytes() > 0, "can't allocated a device buffer of size 0")();

	HRESULT hr;
	D3D11_BUFFER_DESC BufDesc;
	D3D11_SUBRESOURCE_DATA ResData;
	D3D11_SUBRESOURCE_DATA* pResData;

	core::zero_object(BufDesc);
	core::zero_object(ResData);
	pResData = nullptr;

	BufDesc.ByteWidth = pBuf->getSizeInBytes();
	BufDesc.Usage = D3D11_USAGE_DEFAULT;
	BufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	if (flags.IsAnySet())
	{
		BufDesc.Usage = D3D11_USAGE_DYNAMIC;

		if (flags.IsSet(CpuAccess::READ))
			BufDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
		if (flags.IsSet(CpuAccess::WRITE))
			BufDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
	}

	if (pData) // optional data
	{
		ResData.pSysMem = pData;
		pResData = &ResData;
	}


	hr = render::g_Dx11D3D.DxDevice()->CreateBuffer(&BufDesc, pResData, &pBuf->pDevBuf);

	if (FAILED(hr)) {
		X_ERROR("VidMem", "failed to allocated vertex stream of size: %i", pBuf->getSizeInBytes());
	}
	else {
		D3DDebug::SetDebugObjectName(pBuf->pDevBuf, "Vertex Buffer");
	}

	return SUCCEEDED(hr);
}

bool VidMemManager::DeviceCreateIB(X3DBuffer* pBuf, const void* pData, const CpuAccessFlags flags)
{
	X_ASSERT(pBuf->getSizeInBytes() > 0, "can't allocated a device buffer of size 0")();

	HRESULT hr;
	D3D11_BUFFER_DESC BufDesc;
	D3D11_SUBRESOURCE_DATA ResData;
	D3D11_SUBRESOURCE_DATA* pResData;

	core::zero_object(BufDesc);
	core::zero_object(ResData);
	pResData = nullptr;

	BufDesc.ByteWidth = pBuf->getSizeInBytes();
	BufDesc.Usage = D3D11_USAGE_DEFAULT;
	BufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;


	if (flags.IsAnySet())
	{
		BufDesc.Usage = D3D11_USAGE_DYNAMIC;

		if (flags.IsSet(CpuAccess::READ))
			BufDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
		if (flags.IsSet(CpuAccess::WRITE))
			BufDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
	}

	if (pData) // optional data
	{
		ResData.pSysMem = pData;
		pResData = &ResData;
	}

	hr = render::g_Dx11D3D.DxDevice()->CreateBuffer(&BufDesc, pResData, &pBuf->pDevBuf);

	if (FAILED(hr)) {
		X_ERROR("VidMem", "failed to allocated index stream of size: %i", pBuf->getSizeInBytes());
	}
	else {
		D3DDebug::SetDebugObjectName(pBuf->pDevBuf, "Index Buffer");
	}

	return SUCCEEDED(hr);
}

// ---------------------------------------------------------------

void VidMemManager::DeviceUnsetIndexs(X3DBuffer* pBuf) const
{
	// check if this index's are currently bound to the pipeline
	// if so unset.


}

void VidMemManager::DeviceUnsetVertexStream(X3DBuffer* pBuf) const
{
	// check if the vertex stream is currently bound to the pipeline
	// if so unset.

}

// ---------------------------------------------------------------

VidMemManager::Stats VidMemManager::getStats(void) const
{
#if X_ENABLE_VID_MEMORY_STATS
	return stats_;
#else
	static Stats stats;
	return stats;
#endif // !X_ENABLE_VID_MEMORY_STATS
}


X_NAMESPACE_END