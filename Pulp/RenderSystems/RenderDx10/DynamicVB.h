#pragma once

#ifndef X_D3D_DYNAMIC_VERTEX_BUF_H_
#define X_D3D_DYNAMIC_VERTEX_BUF_H_

X_NAMESPACE_BEGIN(render)


template<class VertexType>
class XDynamicVB
{
public:
	X_INLINE XDynamicVB();
	X_INLINE XDynamicVB(VidMemManager* pVidMem, const uint32& vertCount, bool readWrite);
	X_INLINE ~XDynamicVB();

	X_INLINE void Release();


	X_INLINE HRESULT CreateVB(VidMemManager* pVidMem, uint32 vertCount, uint32 stride);
	X_INLINE VertexType* LockVB(const uint32 requested_verts, uint32 &offset, bool write = true);
	X_INLINE void UnlockVB();
	X_INLINE HRESULT Bind(VertexStream::Enum stream = VertexStream::VERT, int bytesOffset = 0, 
		int stride = 0);

private:

	uint32_t vertsNum_;
	uint32_t vertStride_;
	uint32_t offset_;
	bool	 Locked_;
	bool	 ReadWrite_;
	VertexType*   pLockedData_;
	VidMemManager* pVidMem_;
	uint32_t vbId_;
};

template<class VertexType>
XDynamicVB<VertexType>::XDynamicVB() :
	vertsNum_(0),
	vertStride_(0),
	offset_(0),

	Locked_(false),
	ReadWrite_(false),

	pLockedData_(nullptr),
	vbId_(VidMemManager::null_id),
	pVidMem_(nullptr)
{
	vertStride_ = sizeof(VertexType);

}

template<class VertexType>
XDynamicVB<VertexType>::XDynamicVB(VidMemManager* pVidMem, const uint32& vertCount, bool readWrite) :
	vertsNum_(0),
	vertStride_(0),
	offset_(0),

	Locked_(false),
	ReadWrite_(false),

	pLockedData_(nullptr),
	vbId_(VidMemManager::null_id),
	pVidMem_(pVidMem)
{
	ReadWrite_ = readWrite;

	CreateVB(pVidMem, vertCount, sizeof(VertexType));
}

template<class VertexType>
XDynamicVB<VertexType>::~XDynamicVB()
{
	Release();
}

template<class VertexType>
void XDynamicVB<VertexType>::Release()
{
	UnlockVB();

	if (pVidMem_)
	{
		pVidMem_->freeVB(vbId_);
		vbId_ = VidMemManager::null_id;
	}
	else if (vbId_ != VidMemManager::null_id)
	{
		X_ASSERT_UNREACHABLE();
	}
}

template<class VertexType>
HRESULT XDynamicVB<VertexType>::CreateVB(VidMemManager* pVidMem, uint32 vertCount, uint32 stride)
{
	HRESULT hr = S_OK;
	pVidMem_ = pVidMem;

	// set info.
	vertStride_ = stride;
	vertsNum_ = vertCount;


	VidMemManager::CpuAccessFlags cpuflags;

	cpuflags.Set(CpuAccess::WRITE);

	if (ReadWrite_)
		cpuflags.Set(CpuAccess::READ);

	vbId_ = pVidMem->CreateVB(vertsNum_ * vertStride_, cpuflags);
	
	D3DDebug::SetDebugObjectName(pVidMem->getD3DVB(vbId_), "DynamicVB");

	return hr;
}

template<class VertexType>
VertexType* XDynamicVB<VertexType>::LockVB(const uint32 requested_verts, uint32 &offset, bool write)
{
	uint32 LockByteCount;
	MapType::Enum mapType;

	if (requested_verts > vertsNum_) {
		X_FATAL("DynamicVB", "vertex buffer can't satisfy the requested size: %i max: %i",
			requested_verts, vertsNum_);
		return nullptr;
	}

	if (Locked_) {
		X_WARNING("DynamicVB", "vertex buffer already locked.");
		return pLockedData_;
	}

#if 1
	if (vbId_ != VidMemManager::null_id)
	{
		LockByteCount = requested_verts * vertStride_;

		if (ReadWrite_) {
			if (write)
				mapType = MapType::WRITE;
			else
				mapType = MapType::READ;
		}
		else {
			mapType = MapType::WRITE_DISCARD;
		}


		void* pData = pVidMem_->MapVB(vbId_, mapType);

		// we check if we can fit the requested vert count.
		// in the end of the buffer.
		// if not we reset sorta like a ring buffer.
		if (requested_verts + offset_ > vertsNum_)
		{
			pLockedData_ = (VertexType*)pData;

			offset = 0;
			offset_ = requested_verts;
		}
		else
		{
			pLockedData_ = (VertexType*)(((uint8*)pData) + offset_ * vertStride_);
			offset = offset_;
			offset_ += requested_verts;
		}

		Locked_ = true;
	}
#else
	if (pD3DBuffer_)
	{
		LockByteCount = requested_verts * vertStride_;

		if (ReadWrite_) {
			if (write)
				MapType = D3D11_MAP_WRITE;
			else
				MapType = D3D11_MAP_READ;
		}
		else {
			MapType = D3D11_MAP_WRITE_DISCARD;
		}

		g_Dx11D3D.DxDeviceContext()->Map(pD3DBuffer_, 0, MapType, 0, &resource);

		// we check if we can fit the requested vert count.
		// in the end of the buffer.
		// if not we reset sorta like a ring buffer.
		if (requested_verts + offset_ > vertsNum_)
		{
			pLockedData_ = (VertexType*)resource.pData;

			offset = 0;
			offset_ = requested_verts;
		}
		else
		{
			pLockedData_ = (VertexType*)(((uint8*)resource.pData) + offset_ * vertStride_);
			offset = offset_;
			offset_ += requested_verts;
		}

		Locked_ = true;
	}
#endif

	return pLockedData_;
}


template<class VertexType>
void XDynamicVB<VertexType>::UnlockVB()
{
	// no point checking id, since it can only become locked if the id is valid ;)
	if (Locked_) // && vbId_ != VidMemManager::null_id)
	{
		pVidMem_->UnMapVB(vbId_);
	//	g_Dx11D3D.DxDeviceContext()->Unmap(pD3DBuffer_, 0);
		Locked_ = false;
	}
}


template<class VertexType>
HRESULT XDynamicVB<VertexType>::Bind(VertexStream::Enum stream, int bytesOffset, int stride)
{
	g_Dx11D3D.FX_SetVStream(
		vbId_,
		stream,
		stride == 0 ? vertStride_ : stride,
		bytesOffset
		);

	return S_OK;
}

X_NAMESPACE_END;

#endif // !X_D3D_DYNAMIC_VERTEX_BUF_H_