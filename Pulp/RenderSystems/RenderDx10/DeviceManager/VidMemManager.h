#pragma once

#ifndef X_VID_MEM_MANAGER_H_
#define X_VID_MEM_MANAGER_H_

#include <Containers\Array.h>
#include <Containers\Fifo.h>

#include <Memory\HeapArea.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\ThreadPolicies\SingleThreadPolicy.h>
#include <Memory\BoundsCheckingPolicies\NoBoundsChecking.h>
#include <Memory\MemoryTaggingPolicies\NoMemoryTagging.h>
#include <Memory\MemoryTaggingPolicies\SimpleMemoryTagging.h>
#include <Memory\MemoryTrackingPolicies\SimpleMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\MemoryArena.h>


X_NAMESPACE_BEGIN(render)

#define X_ENABLE_VID_MEMORY_STATS 1

X_DECLARE_ENUM(VidBufType)(INDEX, VERTEX);
X_DECLARE_ENUM(MapType)(Invalid, READ,WRITE, READ_WRITE, WRITE_DISCARD, WRITE_NO_OVERWRITE);

X_DECLARE_FLAGS(CpuAccess)(WRITE, READ);

class VidMemManager;

class X3DBuffer // can be either a index or vertex buffer.
{
public:
	X3DBuffer() : sizeBytes(0), pDevBuf(nullptr), pLockedData(nullptr), locked_(false) {}
	~X3DBuffer() {
		X_ASSERT(pDevBuf == nullptr, "device handel not released")();
	}
	X_INLINE uint32_t getSizeInBytes(void) const { return sizeBytes; }
	X_INLINE VidBufType::Enum getBufferType(void) const { return type; }
	X_INLINE ID3D11Buffer* getBuffer(void) const { return pDevBuf; }

	X_INLINE bool isLocked(void) const { return locked_; }
private:
	friend class VidMemManager;

	uint32_t sizeBytes;
	VidBufType::Enum type;
	ID3D11Buffer* pDevBuf;

	void* pLockedData;
	bool locked_;
};


class VidMemManager
{
	friend class DX11XRender;
	template<typename> 
	friend class XDynamicVB;

	typedef core::MemoryArena<core::PoolAllocator, 
		core::SingleThreadPolicy,
		core::NoBoundsChecking, 
#if X_DEBUG
		core::SimpleMemoryTracking,
		core::SimpleMemoryTagging
#else
		core::NoMemoryTracking,
		core::NoMemoryTagging
#endif !X_DEBUG
		> PoolArena;

public:
	enum {
		null_id = (uint32_t)-1
	};

	typedef Flags<CpuAccess> CpuAccessFlags;

	VidMemManager();
	~VidMemManager();

	void StartUp(void);
	void ShutDown(void);


	// we return 32bit id's saves 4 bytes sotrage in 64bit.
	// and the buffer is dx specifix so id's can be exposed to 3d engine etc.
	uint32_t CreateIB(uint32_t size, CpuAccessFlags flags = 0);
	uint32_t CreateIB(uint32_t size, const void* pData, CpuAccessFlags flags = 0);
	uint32_t CreateVB(uint32_t size, CpuAccessFlags flags = 0);
	uint32_t CreateVB(uint32_t size, const void* pData, CpuAccessFlags flags = 0);


	// free from ID
	void freeIB(uint32_t IBid);
	void freeVB(uint32_t VBid);

	// get the buffer from a ID
	X3DBuffer* IBFromId(uint32_t bufID) const;
	X3DBuffer* VBFromId(uint32_t bufID) const;

	void* MapIB(uint32_t IBid, MapType::Enum maptype);
	void* MapVB(uint32_t VBid, MapType::Enum maptype);

	void UnMapIB(uint32_t IBid);
	void UnMapVB(uint32_t VBid);
private:
	// Index util.
	uint32_t createIdForBuffer(X3DBuffer* pBuf);
	X3DBuffer* bufferForId(uint32_t bufID) const;
	void FreeId(uint32_t id);

	// Internal create
	X3DBuffer* Int_CreateVB(uint32_t size);
	X3DBuffer* Int_CreateIB(uint32_t size);

	// Internal Free
	void Int_freeIB(X3DBuffer* pIB);
	void Int_freeVB(X3DBuffer* pVB);

protected:
	// used by render class
	X_INLINE ID3D11Buffer* getD3DIB(uint32_t id);
	X_INLINE ID3D11Buffer* getD3DVB(uint32_t id);
private:

	bool DeviceCreateVB(X3DBuffer* pBuf, const void* pData, const CpuAccessFlags flags);
	bool DeviceCreateIB(X3DBuffer* pBuf, const void* pData, const CpuAccessFlags flags);

	void DeviceUnsetIndexs(X3DBuffer* pBuf) const;
	void DeviceUnsetVertexStream(X3DBuffer* pBuf) const;

public:
	struct Stats 
	{
		Stats();

		uint32_t numIndexBuffers;
		uint32_t numVertexBuffers;
		uint32_t maxIndexBuffers;
		uint32_t maxVertexBuffers;

		uint32_t indexesBytes;
		uint32_t vertexBytes;
		uint32_t maxIndexesBytes;
		uint32_t maxVertexBytes;
	};

	Stats getStats(void) const;

private:
	core::Array<X3DBuffer> VBs_;
	core::Array<X3DBuffer> IBs_;

	core::HeapArea heap_;
	core::PoolAllocator pool_;
	PoolArena arena_;

	core::Array<X3DBuffer*> idLookup_;
	core::Fifo<uint32_t> freeIds_;

#if X_ENABLE_VID_MEMORY_STATS
	Stats stats_;
#endif // !X_ENABLE_VID_MEMORY_STATS
};


// used by render class
ID3D11Buffer* VidMemManager::getD3DIB(uint32_t id)
{
	X3DBuffer* pBuf = IBFromId(id);
	X_ASSERT_NOT_NULL(pBuf);

	return pBuf->getBuffer();
}

ID3D11Buffer* VidMemManager::getD3DVB(uint32_t id)
{
	X3DBuffer* pBuf = VBFromId(id);
	X_ASSERT_NOT_NULL(pBuf);

	return pBuf->getBuffer();
}

X_NAMESPACE_END

#endif // !X_VID_MEM_MANAGER_H_