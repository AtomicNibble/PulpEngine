#include "stdafx.h"

#include <../Common/RenderAux.h>
#include <../Common/Shader/XShader.h>
#include <IShader.h>

#include <Dx10Render.h>

X_NAMESPACE_BEGIN(render)


X_INLINE uint32_t BufferSize(ID3D11Buffer* pBuffer)
{
	X_ASSERT_NOT_NULL(pBuffer);

	D3D11_BUFFER_DESC Desc;
	pBuffer->GetDesc(&Desc);
	return Desc.ByteWidth;
}

class XRenderAuxImp : public IRenderAuxImpl
{
	X_NO_ASSIGN(XRenderAuxImp);
public:
	XRenderAuxImp(render::DX11XRender& renderer);
	~XRenderAuxImp() X_OVERRIDE;

	// IRenderAuxImpl
	virtual void Flush(const XAuxGeomCBRawDataPackaged& data, size_t begin, size_t end) X_OVERRIDE;
	virtual void RT_Flush(const XAuxGeomCBRawDataPackaged& data, size_t begin, size_t end) X_OVERRIDE;
	// ~IRenderAuxImpl


	int GetDeviceDataSize();
	void ReleaseDeviceObjects();
	HRESULT RestoreDeviceObjects();
	void SetOrthoMode(bool enable, Matrix44f* pMatrix = nullptr);

	X_INLINE void ReleaseShader() {
		core::SafeRelease(pAuxGeomShader_);
	}

	X_INLINE XRenderAux* GetRenderAuxGeom() {
		return auxGeomCBCol_.Get(this);
	}

private:
	static const int auxObjNumLOD = 5;
	static const int AuxGeom_VBSize;
	static const int AuxGeom_IBSize;

	struct XMatrices
	{
		XMatrices()
		: pCurTransMat(nullptr)
		{
			matView.setToIdentity();
			matViewInv.setToIdentity();
			matProj.setToIdentity();
			matTrans3D.setToIdentity();

			matTrans2D = Matrix44f(2, 0, 0, 0,
				0, -2, 0, 0,
				0, 0, 0, 0,
				-1, 1, 0, 1);
		}

		void UpdateMatrices(DX11XRender& renderer);

		Matrix44f matView;
		Matrix44f matViewInv;
		Matrix44f matProj;
		Matrix44f matTrans3D;
		Matrix44f matTrans2D;
		const Matrix44f* pCurTransMat;
	};

	struct XDrawObjMesh
	{
		XDrawObjMesh() :
			numVertices(0),
			numFaces(0),
			VBid(VidMemManager::null_id),
			IBid(VidMemManager::null_id)
		{
		}

		~XDrawObjMesh()
		{
			Release();
		}

		void Release()
		{
			g_Dx11D3D.VidMemMng()->freeVB(VBid);
			g_Dx11D3D.VidMemMng()->freeIB(IBid);

			VBid = VidMemManager::null_id;
			IBid = VidMemManager::null_id;
			numVertices = 0;
			numFaces = 0;
		}

		int GetDeviceDataSize()
		{
			int nSize = 0;
		//	nSize += BufferSize(pVB);
		//	nSize += BufferSize(pIB);
			return nSize;
		}

		uint32 numVertices;
		uint32 numFaces;
		uint32 VBid;
		uint32 IBid;
	};

	class XAuxGeomCBCollector
	{
	public:
		XAuxGeomCBCollector() :
			pCBs(nullptr)
		{
		}
		~XAuxGeomCBCollector()
		{
			X_DELETE(pCBs, g_rendererArena);
		}


		XRenderAux* Get(IRenderAuxImpl* pRenderAuxGeomImpl)
		{
			if (!pCBs)
				pCBs = X_NEW(XRenderAux, g_rendererArena, "AuxGeoCB")(pRenderAuxGeomImpl);
			return pCBs;
		}

	private:
		XRenderAux* pCBs;
	};

	struct XStreamBufferManager
	{
	public:
		XStreamBufferManager();
		void Reset();
		void DiscardVB();
		void DiscardIB();

	public:
		bool discardVB;
		bool discardIB;
		bool _pad[2];
		uint32 curVBIndex;
		uint32 curIBIndex;
	};

public:

private:
	void DetermineAuxPrimitveFlags(uint32& d3dNumPrimDivider, PrimitiveType::Enum& d3dPrim, XRenderAux::PrimType::Enum primType) const;
	void DrawAuxPrimitives(XRenderAux::AuxSortedPushBuffer::const_iterator itBegin, XRenderAux::AuxSortedPushBuffer::const_iterator itEnd, const XRenderAux::PrimType::Enum primType);
	void DrawAuxIndexedPrimitives(XRenderAux::AuxSortedPushBuffer::const_iterator itBegin, XRenderAux::AuxSortedPushBuffer::const_iterator itEnd, const XRenderAux::PrimType::Enum primType);
	void DrawAuxObjects(XRenderAux::AuxSortedPushBuffer::const_iterator itBegin, XRenderAux::AuxSortedPushBuffer::const_iterator itEnd);

	void PrepareThickLines2D(XRenderAux::AuxSortedPushBuffer::const_iterator itBegin, XRenderAux::AuxSortedPushBuffer::const_iterator itEnd);
	void PrepareThickLines3D(XRenderAux::AuxSortedPushBuffer::const_iterator itBegin, XRenderAux::AuxSortedPushBuffer::const_iterator itEnd);

	void PrepareRendering();
	bool SetShader(const XAuxGeomRenderFlags& renderFlags);
	void AdjustRenderStates(const XAuxGeomRenderFlags& renderFlags);
	bool BindStreams(shader::VertexFormat::Enum newVertexFormat, 
		uint32_t NewVB, uint32_t NewIB);


	template< typename TMeshFunc >
	bool CreateMesh(XDrawObjMesh& mesh, TMeshFunc meshFunc);

	const Matrix44f& GetCurrentView() const;
	const Matrix44f& GetCurrentViewInv() const;
	const Matrix44f& GetCurrentProj() const;
	const Matrix44f& GetCurrentTrans3D() const;
	const Matrix44f& GetCurrentTrans2D() const;

	bool IsOrthoMode() const;

	const XRenderAux::AuxVertexBuffer& GetAuxVertexBuffer() const;
	const XRenderAux::AuxIndexBuffer& GetAuxIndexBuffer() const;
	const XRenderAux::AuxDrawObjParamBuffer& GetAuxDrawObjParamBuffer() const;
	const Matrix44f& GetAuxOrthoMatrix(int idx) const;

private:
	XStreamBufferManager auxGeomSBM_;

	uint32 auxGeomVB_;
	uint32 auxGeomIB_;
	uint32 curVB_;
	uint32 curIB_;

	uint32 wndXRes_;
	uint32 wndYRes_;
	float aspect_;
	float aspectInv_;

	XMatrices matrices_;

	XRenderAux::PrimType::Enum curPrimType_;
	int curTransMatrixIdx_;

	uint8 curPointSize_;
	uint8 _pad[3];;

	int CV_r_auxGeom_;


	shader::XShader* pAuxGeomShader_;
	AuxGeom_DrawInFrontMode::Enum curDrawInFrontMode_;

	XRenderAux::AuxSortedPushBuffer auxSortedPushBuffer_;
	const XRenderAux::XAuxGeomCBRawData* pCurCBRawData_;
	XAuxGeomCBCollector auxGeomCBCol_;

	XDrawObjMesh sphereObj_[auxObjNumLOD];
	XDrawObjMesh coneObj_[auxObjNumLOD];
	XDrawObjMesh cylinderObj_[auxObjNumLOD];

	render::VidMemManager* pVidMemManager_;
	render::DX11XRender& renderer_;
};


struct XAuxObjVertex
{
	XAuxObjVertex()	{}

	XAuxObjVertex(const Vec3f& pos_, const Vec3f& normal_) :
		pos(pos_),
		normal(normal_)
	{
	}

	Vec3f pos;
	Vec3f normal;
};


X_NAMESPACE_END