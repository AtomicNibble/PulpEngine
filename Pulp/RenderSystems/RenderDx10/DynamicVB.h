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
	X_INLINE HRESULT Bind(uint32 streamNumber = 0, int bytesOffset = 0, int stride = 0);

private:

	uint32_t vertsNum_;
	uint32_t vertStride_;
	uint32_t offset_;
	bool	 Locked_;
	bool	 ReadWrite_;
	VertexType*   pLockedData_;
//	ID3D11Buffer* pD3DBuffer_;
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
//	if (pD3DBuffer_) {
//		pD3DBuffer_->Release();
//		pD3DBuffer_ = nullptr;
//	}
}

template<class VertexType>
HRESULT XDynamicVB<VertexType>::CreateVB(VidMemManager* pVidMem, uint32 vertCount, uint32 stride)
{
	HRESULT hr = S_OK;
//	D3D11_BUFFER_DESC BufDesc;
//	core::zero_object(BufDesc);
	pVidMem_ = pVidMem;

	// set info.
	vertStride_ = stride;
	vertsNum_ = vertCount;
/*
	BufDesc.ByteWidth = vertsNum_*vertStride_;
	BufDesc.Usage = D3D11_USAGE_DYNAMIC;
	BufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufDesc.MiscFlags = 0;

	if (ReadWrite_)
	{
		BufDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
		BufDesc.Usage = D3D11_USAGE_STAGING;
		BufDesc.BindFlags = 0;
	}

	hr = pD3D->CreateBuffer(&BufDesc, NULL, &pD3DBuffer_);
	*/

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
HRESULT XDynamicVB<VertexType>::Bind(uint32 streamNumber, int bytesOffset, int stride)
{
	g_Dx11D3D.FX_SetVStream(
		vbId_,
		streamNumber,
		stride == 0 ? vertStride_ : stride,
		bytesOffset
		);

	return S_OK;
}

X_NAMESPACE_END;

#endif // !X_D3D_DYNAMIC_VERTEX_BUF_H_