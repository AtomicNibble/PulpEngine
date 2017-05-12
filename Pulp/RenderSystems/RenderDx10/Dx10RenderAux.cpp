#include "stdafx.h"

#include "Dx10RenderAux.h"

#include <IConsole.h>


X_NAMESPACE_BEGIN(render)


namespace{

	static Matrix44f identiy_mat(Matrix44f::identity());

	const float c_clipThres = 0.1f;

	static inline Vec3f IntersectLinePlane(const Vec3f& o,
		const Vec3f& d, const Planef& p, float& t)
	{
		t = -(p.getNormal().dot(o) + (p.getDistance() + c_clipThres)) / p.getNormal().dot(d);
		return (o + d * t);
	}


	static inline Color8u ClipColor(const Color8u& c0, const Color8u& c1, float t)
	{
		Color8u v0(c0);
		return (c0 + (c1 - v0) * t);
	}


	static bool ClipLine(Vec3f* v, Color8u* c)
	{
		// get near plane to perform clipping	
		Planef nearPlane = gRenDev->GetCamera().getFrustumPlane(FrustumPlane::NEAR);


		// get clipping flags
		bool bV0Behind = (-(nearPlane.getNormal().dot(v[0]) + nearPlane.getDistance()) < c_clipThres);
		bool bV1Behind = (-(nearPlane.getNormal().dot(v[1]) + nearPlane.getDistance()) < c_clipThres);


	//	bool bV0Behind(-(D3DXPlaneDotNormal(&nearPlane, &v[0]) + nearPlane.d) < c_clipThres);
	//	bool bV1Behind(-(D3DXPlaneDotNormal(&nearPlane, &v[1]) + nearPlane.d) < c_clipThres);

		// proceed only if both are not behind near clipping plane
		if (false == bV0Behind || false == bV1Behind)
		{
			if (false == bV0Behind && false == bV1Behind)
			{
				// no clipping needed
				return true;
			}

			// define line to be clipped
			Vec3f p(v[0]);
			Vec3f d(v[1] - v[0]);

			// get clipped position
			float t = 0.f;
			v[0] = (false == bV0Behind) ? v[0] : IntersectLinePlane(p, d, nearPlane, t);
			v[1] = (false == bV1Behind) ? v[1] : IntersectLinePlane(p, d, nearPlane, t);

			// get clipped colors
			c[0] = (false == bV0Behind) ? c[0] : ClipColor(c[0], c[1], t);
			c[1] = (false == bV1Behind) ? c[1] : ClipColor(c[0], c[1], t);

			return true;
		}

		return false;
	}


	static float ComputeConstantScale(const Vec3f& v,
		const Matrix44f& matView, const Matrix44f& matProj, const uint32 wndXRes)
	{
		Vec4f vCam0;

		vCam0 = matView * v;
	//	mathVec3TransformF(&vCam0, &v, &matView);

		Vec4f vCam1(vCam0);
		vCam1.x += 1.0f;

		float d0(vCam0.x * matProj.m03 +
			vCam0.y * matProj.m13 +
			vCam0.z * matProj.m23 +
			matProj.m33);

		if (d0 == 0.0f)
		{
			d0 = FLT_EPSILON;
		}

		float c0((vCam0.x * matProj.m00 +
			vCam0.y * matProj.m10 +
			vCam0.z * matProj.m20 +
			matProj.m30) / d0);

		float d1(vCam1.x * matProj.m03 +
			vCam1.y * matProj.m13 +
			vCam1.z * matProj.m23 +
			matProj.m33);

		if (d1 == 0.0f)
		{
			d1 = FLT_EPSILON;
		}

		float c1((vCam1.x * matProj.m00 +
			vCam1.y * matProj.m10 +
			vCam1.z * matProj.m20 +
			matProj.m30) / d1);

		const float epsilon = 0.001f;
		float s = (float)wndXRes * (c1 - c0);
		return (math<float>::abs(s) >= epsilon) ? 1.0f / s : 1.0f / epsilon;
	}


}

// const int XRenderAuxImp::auxObjNumLOD = 5;
const int XRenderAuxImp::AuxGeom_VBSize = 32768;
const int XRenderAuxImp::AuxGeom_IBSize = AuxGeom_VBSize * 2;


XRenderAuxImp::XRenderAuxImp(render::DX11XRender& renderer) :
	renderer_(renderer),
	auxGeomVB_(VidMemManager::null_id),
	auxGeomIB_(VidMemManager::null_id),
	curVB_(VidMemManager::null_id),
	curIB_(VidMemManager::null_id),
	auxGeomSBM_(),
	wndXRes_(0),
	wndYRes_(0),
	aspect_(1.0f),
	aspectInv_(1.0f),
	matrices_(),
	curPrimType_(XRenderAux::PrimType::Invalid),
	curPointSize_(1),
	curTransMatrixIdx_(-1),
	pAuxGeomShader_(nullptr),
	curDrawInFrontMode_(AuxGeom_DrawInFrontMode::DrawInFrontOff),
	auxSortedPushBuffer_(),
	pCurCBRawData_(nullptr),
	auxGeomCBCol_(),
	CV_r_auxGeom_(1),
	pVidMemManager_(nullptr)
{
	pVidMemManager_ = renderer.VidMemMng();

	X_ASSERT_NOT_NULL(pVidMemManager_);

	// register a cvar baby.
	ADD_CVAR_REF("r_auxgeo", CV_r_auxGeom_, 1, 0, 1, 0, "draw aux geo");
}

XRenderAuxImp::~XRenderAuxImp()
{

}

// IRenderAuxImpl
void XRenderAuxImp::Flush(const XAuxGeomCBRawDataPackaged& data, size_t begin, size_t end)
{
	RT_Flush(data, begin, end);
}

void XRenderAuxImp::RT_Flush(const XAuxGeomCBRawDataPackaged& data, size_t begin, size_t end)
{
	if (!CV_r_auxGeom_)
		return;

	X_PROFILE_BEGIN("AuxGeom", core::profiler::SubSys::RENDER);

	X_ASSERT_NOT_NULL(data.pData_);

	pCurCBRawData_ = data.pData_;


	renderer_.PushViewMatrix();

	if (!renderer_.IsDeviceLost())
	{
		// prepare rendering
		PrepareRendering();

		// get push buffer to process all submitted auxiliary geometries
		pCurCBRawData_->GetSortedPushBuffer(begin, end, auxSortedPushBuffer_);

		// process push buffer
		for (XRenderAux::AuxSortedPushBuffer::const_iterator 
			it(auxSortedPushBuffer_.begin()), itEnd(auxSortedPushBuffer_.end()); it != itEnd;)
		{
			// mark current push buffer position
			XRenderAux::AuxSortedPushBuffer::const_iterator itCur(it);

			// get current render flags
			const XAuxGeomRenderFlags& curRenderFlags((*itCur)->renderFlags);
			curTransMatrixIdx_ = (*itCur)->transMatrixIdx;

			// get prim type
			XRenderAux::PrimType::Enum primType(XRenderAux::GetPrimType(curRenderFlags));

			// find all entries sharing the same render flags
			X_DISABLE_WARNING(4127)
			while (true)
			X_ENABLE_WARNING(4127)
			{
				++it;
				if ((it == itEnd) || ((*it)->renderFlags != curRenderFlags) 
					|| ((*it)->transMatrixIdx != curTransMatrixIdx_))
				{
					break;
				}
			}

			// prepare thick lines
			if (XRenderAux::PrimType::TriList == primType && XRenderAux::IsThickLine(curRenderFlags))
			{
				if (AuxGeom_Mode2D3D::Mode3D == curRenderFlags.GetMode2D3DFlag())
				{
					PrepareThickLines3D(itCur, it);
				}
				else
				{
					PrepareThickLines2D(itCur, it);
				}
			}

			// set appropriate shader
			if (!SetShader(curRenderFlags)) {
				renderer_.PopViewMatrix();
				return;
			}

			// draw push buffer entries
			switch (primType)
			{
				case XRenderAux::PrimType::PtList:
				case XRenderAux::PrimType::LineList:
				case XRenderAux::PrimType::TriList:
				{
					DrawAuxPrimitives(itCur, it, primType);
					break;
				}
				case XRenderAux::PrimType::LineListInd:
				case XRenderAux::PrimType::TriListInd:
				{
					DrawAuxIndexedPrimitives(itCur, it, primType);
					break;
				}
				case XRenderAux::PrimType::Obj:
				default:
				{
					DrawAuxObjects(itCur, it);
					break;
				}
			}
			
		}
		
	}

	renderer_.PopViewMatrix();


	pCurCBRawData_ = nullptr;
	curTransMatrixIdx_ = 0;
}
// ~IRenderAuxImpl

// ----------------------------------------------------------------

void XRenderAuxImp::DetermineAuxPrimitveFlags(uint32& d3dNumPrimDivider,
	PrimitiveType::Enum& ePrimType, XRenderAux::PrimType::Enum primType) const
{
	switch (primType)
	{
		case XRenderAux::PrimType::PtList:
		{
			d3dNumPrimDivider = 1;
			ePrimType = PrimitiveType::PointList;
			break;
		}
		case XRenderAux::PrimType::LineList:
		case XRenderAux::PrimType::LineListInd:
		{
			d3dNumPrimDivider = 2;
			ePrimType = PrimitiveType::LineList;
			break;
		}
		case XRenderAux::PrimType::TriList:
		case XRenderAux::PrimType::TriListInd:
		default:
		{
			d3dNumPrimDivider = 3;
			ePrimType = PrimitiveType::TriangleList;
			break;
		}
	}
}

void XRenderAuxImp::DrawAuxPrimitives(XRenderAux::AuxSortedPushBuffer::const_iterator itBegin,
	XRenderAux::AuxSortedPushBuffer::const_iterator itEnd,
	const XRenderAux::PrimType::Enum primType)
{
	X_ASSERT(XRenderAux::PrimType::PtList == primType || XRenderAux::PrimType::LineList == primType || XRenderAux::PrimType::TriList == primType, "invalid primative type")(primType);

	bool streamsBound = false;

	// bind vertex and index streams and set vertex declaration
	streamsBound = BindStreams(shader::VertexFormat::P3F_T2F_C4B, auxGeomVB_, auxGeomIB_);


	// get aux vertex buffer
	const XRenderAux::AuxVertexBuffer& auxVertexBuffer(GetAuxVertexBuffer());

	// determine flags for prim type
	uint32 d3dNumPrimDivider;
	PrimitiveType::Enum ePrimType;

	DetermineAuxPrimitveFlags(d3dNumPrimDivider, ePrimType, primType);

	// helpers for DP call
	uint32 initialVBLockOffset(auxGeomSBM_.curVBIndex);
	uint32 numVerticesWrittenToVB(0);

	//	m_renderer.FX_Commit();
	// renderer_.FX_ComitParams();

	// process each entry
	for (XRenderAux::AuxSortedPushBuffer::const_iterator it(itBegin); it != itEnd; ++it)
	{
		// get current push buffer entry
		const XRenderAux::XAuxPushBufferEntry* curPBEntry(*it);

		// number of vertices to copy
		uint32 verticesToCopy(curPBEntry->numVertices);
		uint32 verticesCopied(0);

		// stream vertex data
		while (verticesToCopy > 0)
		{
			// number of vertices which fit into current vb
			uint32 maxVerticesInThisBatch(AuxGeom_VBSize - auxGeomSBM_.curVBIndex);

			// round down to previous multiple of "d3dNumPrimDivider"
			maxVerticesInThisBatch -= maxVerticesInThisBatch % d3dNumPrimDivider;

			// still enough space to feed data in the current vb
			if (maxVerticesInThisBatch > 0)
			{
				// compute amount of vertices to move in this batch
				uint32 toCopy(verticesToCopy > maxVerticesInThisBatch ? maxVerticesInThisBatch : verticesToCopy);

				// get pointer to vertex buffer
				XAuxVertex* pVertices = nullptr;

				// determine lock flags
				MapType::Enum mapType = MapType::WRITE_NO_OVERWRITE;
				if (auxGeomSBM_.discardVB)
				{
					auxGeomSBM_.discardVB = false;
					mapType = MapType::WRITE_DISCARD;
				}

				pVertices = (XAuxVertex*)pVidMemManager_->MapVB(auxGeomVB_, mapType);
				if (!pVertices)
				{
					X_ASSERT_UNREACHABLE();
					X_ERROR("AuxGeo", "failed to lock Vertex buffer");
					return;
				}
				
				pVertices += auxGeomSBM_.curVBIndex;


				// move vertex data
				memcpy(pVertices, &auxVertexBuffer[curPBEntry->vertexOffs + verticesCopied], toCopy * sizeof(XAuxVertex));

				// unlock vb
				pVidMemManager_->UnMapVB(auxGeomVB_);

				// update accumulators and buffer indices
				verticesCopied += toCopy;
				verticesToCopy -= toCopy;

				auxGeomSBM_.curVBIndex += toCopy;
				numVerticesWrittenToVB += toCopy;
			}
			else
			{
				// not enough space in vb for (remainder of) current push buffer entry
				if (numVerticesWrittenToVB > 0)
				{
					// commit batch 
					X_ASSERT(0 == numVerticesWrittenToVB % d3dNumPrimDivider, "invalid indice count written")(numVerticesWrittenToVB, d3dNumPrimDivider);

					if (streamsBound)
						renderer_.FX_DrawPrimitive(ePrimType, initialVBLockOffset, numVerticesWrittenToVB);
				}

				// request a DISCARD lock of vb in the next run
				auxGeomSBM_.DiscardVB();
				initialVBLockOffset = auxGeomSBM_.curVBIndex;
				numVerticesWrittenToVB = 0;
			}
		}
	}

	if (numVerticesWrittenToVB > 0)
	{
		// commit batch 
		X_ASSERT(0 == numVerticesWrittenToVB % d3dNumPrimDivider, "invalid indice count written")(numVerticesWrittenToVB, d3dNumPrimDivider);


		if (streamsBound)
			renderer_.FX_DrawPrimitive(ePrimType, initialVBLockOffset, numVerticesWrittenToVB);
	}
}

void XRenderAuxImp::DrawAuxIndexedPrimitives(XRenderAux::AuxSortedPushBuffer::const_iterator itBegin,
	XRenderAux::AuxSortedPushBuffer::const_iterator itEnd,
	const XRenderAux::PrimType::Enum primType)
{
	X_ASSERT(XRenderAux::PrimType::LineListInd == primType || XRenderAux::PrimType::TriListInd == primType, "invalid prim type")(primType);

	bool streamsBound = false;

	// bind vertex and index streams and set vertex declaration
	streamsBound = BindStreams(shader::VertexFormat::P3F_T2F_C4B, auxGeomVB_, auxGeomIB_);
	

	// get aux vertex and index buffer
	const XRenderAux::AuxVertexBuffer& auxVertexBuffer(GetAuxVertexBuffer());
	const XRenderAux::AuxIndexBuffer& auxIndexBuffer(GetAuxIndexBuffer());

	// determine flags for prim type
	uint32 d3dNumPrimDivider;
	PrimitiveType::Enum ePrimType;
	DetermineAuxPrimitveFlags(d3dNumPrimDivider, ePrimType, primType);

	// helpers for DP call
	uint32 initialVBLockOffset(auxGeomSBM_.curVBIndex);
	uint32 numVerticesWrittenToVB;
	uint32 initialIBLockOffset(auxGeomSBM_.curIBIndex);
	uint32 numIndicesWrittenToIB;

	numVerticesWrittenToVB = 0;
	numIndicesWrittenToIB = 0;

//	m_renderer.FX_Commit();
//	renderer_.FX_ComitParams();

	// process each entry
	for (XRenderAux::AuxSortedPushBuffer::const_iterator it(itBegin); it != itEnd;)
	{
		// get current push buffer entry
		const XRenderAux::XAuxPushBufferEntry* curPBEntry(*it);

		// process a push buffer entry if it can fit at all (otherwise silently skip it)
		if (AuxGeom_VBSize >= curPBEntry->numVertices && AuxGeom_IBSize >= curPBEntry->numIndices)
		{
			// check if push buffer still fits into current buffer
			if (AuxGeom_VBSize >= auxGeomSBM_.curVBIndex + curPBEntry->numVertices
				&& AuxGeom_IBSize >= auxGeomSBM_.curIBIndex + curPBEntry->numIndices)
			{
				// determine lock vb flags

				// get pointer to vertex buffer
				XAuxVertex* pVertices = nullptr;
				MapType::Enum mp;
				
				mp = MapType::WRITE_NO_OVERWRITE;
				if (auxGeomSBM_.discardVB)
				{
					auxGeomSBM_.discardVB = false;
					mp = MapType::WRITE_DISCARD;
				}

				pVertices = (XAuxVertex*)pVidMemManager_->MapVB(auxGeomVB_, mp);
				if (!pVertices)
				{
					X_ERROR("AuxGeo", "failed to lock Vertex buffer");
					return;
				}

				pVertices += auxGeomSBM_.curVBIndex;

				// move vertex data of this entry
				memcpy(pVertices, &auxVertexBuffer[curPBEntry->vertexOffs], curPBEntry->numVertices * sizeof(XAuxVertex));

				// unlock vb
				pVidMemManager_->UnMapVB(auxGeomVB_);

				// get pointer to index buffer
				uint16_t* pIndices = nullptr;
				uint32 i;

				mp = MapType::WRITE_NO_OVERWRITE;
				if (auxGeomSBM_.discardIB)
				{
					auxGeomSBM_.discardIB = false;
					mp = MapType::WRITE_DISCARD;
				}

				pIndices = (uint16_t*)pVidMemManager_->MapIB(auxGeomIB_, mp);
				if (!pIndices)
				{
					X_ERROR("AuxGeo", "failed to lock Index buffer");
					return;
				}

				pIndices += auxGeomSBM_.curIBIndex;


				// move index data of this entry (modify indices to match VB insert location)
				for (i=0; i < curPBEntry->numIndices; ++i)
				{
					pIndices[i] = safe_static_cast<uint16_t, uint32_t>(numVerticesWrittenToVB + 
						auxIndexBuffer[curPBEntry->indexOffs + i]);
				}

				// unlock ib
				pVidMemManager_->UnMapIB(auxGeomIB_);

				// update buffer indices
				auxGeomSBM_.curVBIndex += curPBEntry->numVertices;
				auxGeomSBM_.curIBIndex += curPBEntry->numIndices;

				numVerticesWrittenToVB += curPBEntry->numVertices;
				numIndicesWrittenToIB += curPBEntry->numIndices;

				// advance to next push puffer entry
				++it;
			}
			else
			{
				// push buffer entry currently doesn't fit, will be processed in the next iteration when buffers got flushed
				if (numVerticesWrittenToVB > 0 && numIndicesWrittenToIB > 0)
				{
					// commit batch 
					X_ASSERT(0 == numIndicesWrittenToIB % d3dNumPrimDivider, "invalid indice count written")(numIndicesWrittenToIB, d3dNumPrimDivider);

					if (streamsBound) 
					{
						renderer_.FX_DrawIndexPrimitive(ePrimType, numIndicesWrittenToIB,
							initialIBLockOffset, initialVBLockOffset);
					}
				}

				// request a DISCARD lock / don't advance iterator!
				auxGeomSBM_.DiscardVB();
				initialVBLockOffset = auxGeomSBM_.curVBIndex;
				numVerticesWrittenToVB = 0;

				auxGeomSBM_.DiscardIB();
				initialIBLockOffset = auxGeomSBM_.curIBIndex;
				numIndicesWrittenToIB = 0;
			}
		}
		else
		{
			// push buffer entry too big for dedicated vb/ib buffer
			// advance to next push puffer entry
			X_ASSERT_UNREACHABLE();
			X_ERROR("AuxGeo", "geometry too big to render");
			++it;
		}
	}

	if (numVerticesWrittenToVB > 0 && numIndicesWrittenToIB > 0)
	{
		// commit batch 
		X_ASSERT(0 == numIndicesWrittenToIB % d3dNumPrimDivider, "invalid indice count written")(numIndicesWrittenToIB, d3dNumPrimDivider);

		if (streamsBound) 
		{
			renderer_.FX_DrawIndexPrimitive(ePrimType, numIndicesWrittenToIB,
				initialIBLockOffset, initialVBLockOffset);
		}
	}
}



void XRenderAuxImp::DrawAuxObjects(XRenderAux::AuxSortedPushBuffer::const_iterator itBegin,
	XRenderAux::AuxSortedPushBuffer::const_iterator itEnd)
{
	XRenderAux::DrawObjType::Enum objType(XRenderAux::GetAuxObjType((*itBegin)->renderFlags));

	// get draw params buffer
	const XRenderAux::AuxDrawObjParamBuffer& auxDrawObjParamBuffer(GetAuxDrawObjParamBuffer());

	Color8u lastCol;
	bool lastShaded = false;

	// process each entry
	XRenderAux::AuxSortedPushBuffer::const_iterator it = itBegin;

	for (; it != itEnd; ++it)
	{
		// get current push buffer entry
		const XRenderAux::XAuxPushBufferEntry* curPBEntry = *it;

		// assert than all objects in this batch are of same type
		X_ASSERT(XRenderAux::GetAuxObjType(curPBEntry->renderFlags) == objType, "invalid object type")(objType);

		uint32 drawParamOffs = 0;
		if (curPBEntry->GetDrawParamOffs(drawParamOffs))
		{
			// get draw params
			const XRenderAux::XAuxDrawObjParams& drawParams(auxDrawObjParamBuffer[drawParamOffs]);

			if (drawParamOffs == 0)
			{
				lastCol = drawParams.color;
				lastCol.r = ~lastCol.r;

				lastShaded = !drawParams.shaded;
			}

			// Prepare d3d world space matrix in draw param structure 
			// Attention: in d3d terms matWorld is actually matWorld^T
			Matrix44f matWorld(drawParams.matWorld);
	
			// set transformation matrices
			core::StrHash matWorldViewProjName("matWorldViewProj");
			if (curDrawInFrontMode_ == AuxGeom_DrawInFrontMode::DrawInFrontOn)
			{
				Matrix44f matScale(Matrix44f::createScale(Vec3f(0.999f, 0.999f, 0.999f)));

				Matrix44f matWorldViewScaleProjT;
				matWorldViewScaleProjT = GetCurrentView() * matScale;
				matWorldViewScaleProjT = matWorldViewScaleProjT * GetCurrentProj();

				matWorldViewScaleProjT = matWorldViewScaleProjT.transposed();
				matWorldViewScaleProjT = matWorldViewScaleProjT * matWorld;
				pAuxGeomShader_->FXSetVSFloat(matWorldViewProjName,
					reinterpret_cast<Vec4f*>(&matWorldViewScaleProjT), 4);
			}
			else
			{
				Matrix44f matWorldViewProjT;
				matWorldViewProjT = *matrices_.pCurTransMat;
				matWorldViewProjT = matWorldViewProjT * matWorld;
				pAuxGeomShader_->FXSetVSFloat(matWorldViewProjName,
					reinterpret_cast<Vec4f*>(&matWorldViewProjT), 4);
			}


			// set color
			if (lastCol != drawParams.color) {
				Color colVec(drawParams.color);
				core::StrHash auxGeomObjColorName("auxGeomObjColor");
				pAuxGeomShader_->FXSetVSFloat(auxGeomObjColorName, (Vec4f*)&colVec, 1);
			}

			// set shading flag
			if (lastShaded != drawParams.shaded) {
				Vec4f shadingVec(drawParams.shaded ? 0.4f : 0, drawParams.shaded ? 0.6f : 1, 0, 0);
				core::StrHash auxGeomObjShadingName("auxGeomObjShading");
				pAuxGeomShader_->FXSetVSFloat(auxGeomObjShadingName, &shadingVec, 1);
			}

			// set light vector (rotate back into local space)
			Matrix33f matWorldInv(drawParams.matWorld.inverted());
			Vec3f lightLocalSpace(matWorldInv * Vec3f(0.5773f, 0.5773f, 0.5773f));

			// normalize light vector (matWorld could contain non-uniform scaling)
			lightLocalSpace.normalize();
			Vec4f lightVec(lightLocalSpace.x, lightLocalSpace.y, lightLocalSpace.z, 0.0f);
			core::StrHash globalLightLocalName("globalLightLocal");
			pAuxGeomShader_->FXSetVSFloat(globalLightLocalName, &lightVec, 1);

			// LOD calculation
			Matrix44f matWorldT;
			matWorldT = matWorld.transposed();

			Vec4f objCenterWorld;
			Vec3f nullVec(0.0f, 0.0f, 0.0f);
			objCenterWorld = matWorldT * nullVec;
			Vec4f objOuterRightWorld(objCenterWorld + (Vec4f(GetCurrentView().m00, GetCurrentView().m10, GetCurrentView().m20, 0.0f) * drawParams.size));

			Vec4f v0, v1;
			Vec3f objCenterWorldVec(objCenterWorld.xyz());
			Vec3f objOuterRightWorldVec(objOuterRightWorld.xyz());
		
			v0 = (*matrices_.pCurTransMat) * objCenterWorldVec;
			v1 = (*matrices_.pCurTransMat) * objOuterRightWorldVec;


			float scale;
			X_ASSERT(math<float>::abs(v0.w - v0.w) < 1e-4, "invalid")(math<float>::abs(v0.w - v0.w));
			if (math<float>::abs(v0.w) < 1e-2)
			{
				scale = 0.5f;
			}
			else
			{
				scale = ((v1.x - v0.x) / v0.w) * (float)core::Max(wndXRes_, wndYRes_) / 500.0f;
			}

			// map scale to detail level
			uint32 lodLevel((uint32)((scale / 0.5f) * (auxObjNumLOD - 1)));
			if (lodLevel >= auxObjNumLOD)
			{
				lodLevel = auxObjNumLOD - 1;
			}
			lodLevel = 1;
	
			// get appropriate mesh
			X_ASSERT(lodLevel >= 0 && lodLevel < auxObjNumLOD, "invalid LOD level")(lodLevel);
			XDrawObjMesh* pMesh = nullptr;
			switch (objType)
			{
				case XRenderAux::DrawObjType::Sphere:
				default:
				{
					pMesh = &sphereObj_[lodLevel];
					break;
				}
				case XRenderAux::DrawObjType::Cone:
				{
					pMesh = &coneObj_[lodLevel];
					break;
				}
				case XRenderAux::DrawObjType::Cylinder:
				{
					pMesh = &cylinderObj_[lodLevel];
					break;
				}
			}
			X_ASSERT_NOT_NULL(pMesh);

			// bind vertex and index streams and set vertex declaration
			if (BindStreams(shader::VertexFormat::P3F_T3F, pMesh->VBid, pMesh->IBid))
			{
			//	renderer_.FX_Commit();
				renderer_.FX_ComitParams();

				// draw mesh
				renderer_.FX_DrawIndexPrimitive(render::PrimitiveType::TriangleList,
					pMesh->numFaces * 3, 0, 0);
			}
		}
		else
		{
			X_ASSERT_UNREACHABLE();
		}
	}
}

// ----------------------------------------------------------------

void XRenderAuxImp::PrepareThickLines2D(XRenderAux::AuxSortedPushBuffer::const_iterator itBegin,
	XRenderAux::AuxSortedPushBuffer::const_iterator itEnd)
{
	const XRenderAux::AuxVertexBuffer& auxVertexBuffer(GetAuxVertexBuffer());

	// process each entry
	for (XRenderAux::AuxSortedPushBuffer::const_iterator it(itBegin); it != itEnd; ++it)
	{
		// get current push buffer entry
		const XRenderAux::XAuxPushBufferEntry* curPBEntry = *it;

		uint32 offset = curPBEntry->vertexOffs;
		uint32 i;
		for (i=0; i < curPBEntry->numVertices / 6; ++i, offset += 6)
		{
			// get line vertices and thickness parameter
			const Vec3f v[2] =
			{
				auxVertexBuffer[offset + 0].pos,
				auxVertexBuffer[offset + 1].pos
			};
			const Color8u col[2] =
			{
				auxVertexBuffer[offset + 0].color,
				auxVertexBuffer[offset + 1].color
			};
			float thickness = auxVertexBuffer[offset + 2].pos.x;

			// get line delta and aspect ratio corrected normal
			Vec3f delta(v[1] - v[0]);
			Vec3f normalVec(-delta.y * aspectInv_, delta.x * aspect_, 0.0f);

			// normalize and scale to line thickness
			normalVec.normalize();
			normalVec *= thickness * 0.001f;
		//	mathVec3NormalizeF(&normalVec, &normalVec);
		//	D3DXVECTOR3 normal(normalVec.x, normalVec.y, normalVec.z);
		//	normal *= thickness * 0.001f;

			// compute final 2D vertices of thick line in normalized device space
			Vec3f vf[4];
			vf[0] = v[0] + normalVec;
			vf[1] = v[1] + normalVec;
			vf[2] = v[1] - normalVec;
			vf[3] = v[0] - normalVec;

			// copy data to vertex buffer
			XAuxVertex* pVertices(const_cast<XAuxVertex*>(&auxVertexBuffer[offset]));
			pVertices[0].pos = vf[0];
			pVertices[0].color = col[0];
			pVertices[1].pos = vf[1];
			pVertices[1].color = col[1];
			pVertices[2].pos = vf[2];
			pVertices[2].color = col[1];
			pVertices[3].pos = vf[0];
			pVertices[3].color = col[0];
			pVertices[4].pos = vf[2];
			pVertices[4].color = col[1];
			pVertices[5].pos = vf[3];
			pVertices[5].color = col[0];
		}
	}
}

void XRenderAuxImp::PrepareThickLines3D(XRenderAux::AuxSortedPushBuffer::const_iterator itBegin,
	XRenderAux::AuxSortedPushBuffer::const_iterator itEnd)
{
	const XRenderAux::AuxVertexBuffer& auxVertexBuffer(GetAuxVertexBuffer());

	// process each entry
	for (XRenderAux::AuxSortedPushBuffer::const_iterator it(itBegin); it != itEnd; ++it)
	{
		// get current push buffer entry
		const XRenderAux::XAuxPushBufferEntry* curPBEntry = *it;

		uint32 offset = curPBEntry->vertexOffs;
		uint32 i;
		for (i = 0; i < curPBEntry->numVertices / 6; ++i, offset += 6)
		{
			// get line vertices and thickness parameter
			Vec3f v[2] =
			{
				auxVertexBuffer[offset + 0].pos,
				auxVertexBuffer[offset + 1].pos
			};
			Color8u col[2] =
			{
				auxVertexBuffer[offset + 0].color,
				auxVertexBuffer[offset + 1].color
			};

			float thickness = auxVertexBuffer[offset + 2].pos.x;

			bool skipLine = false;
			Vec4f vf[4];

			if (false == IsOrthoMode()) // regular, 3d projected geometry
			{
				skipLine = !ClipLine(v, col);
				if (false == skipLine)
				{
					// compute depth corrected thickness of line end points
					float thicknessV0(0.5f * thickness * ComputeConstantScale(v[0], GetCurrentView(), GetCurrentProj(), wndXRes_));
					float thicknessV1(0.5f * thickness * ComputeConstantScale(v[1], GetCurrentView(), GetCurrentProj(), wndXRes_));

					// compute camera space line delta
					Vec4f vt[2];

					vt[0] = GetCurrentView() * v[0];
					vt[1] = GetCurrentView() * v[1];

			//		mathVec3TransformF(&vt[0], &v[0], &GetCurrentView());
			//		mathVec3TransformF(&vt[1], &v[1], &GetCurrentView());
					if (vt[0].z != 0.0f && vt[1].z != 0.0f)
					{
						vt[0].z = fsel(-vt[0].z - c_clipThres, vt[0].z, -c_clipThres);
						vt[1].z = fsel(-vt[1].z - c_clipThres, vt[1].z, -c_clipThres);

						Vec2f delta(((vt[1] / vt[1].z) - (vt[0] / vt[0].z)).xy());
					//	Vec2f delta;

						// create screen space normal of line delta
						Vec2f normalVec(-delta.y, delta.x);
						normalVec.normalize();

						Vec2f n[2];
						n[0] = normalVec * thicknessV0;
						n[1] = normalVec * thicknessV1;

						// compute final world space vertices of thick line
						Vec4f vertices[4] =
						{
							Vec4f(vt[0].x + n[0].x, vt[0].y + n[0].y, vt[0].z, vt[0].w),
							Vec4f(vt[1].x + n[1].x, vt[1].y + n[1].y, vt[1].z, vt[1].w),
							Vec4f(vt[1].x - n[1].x, vt[1].y - n[1].y, vt[1].z, vt[1].w),
							Vec4f(vt[0].x - n[0].x, vt[0].y - n[0].y, vt[0].z, vt[0].w)
						};


						vf[0] = GetCurrentViewInv() * vertices[0];
						vf[1] = GetCurrentViewInv() * vertices[1];
						vf[2] = GetCurrentViewInv() * vertices[2];
						vf[3] = GetCurrentViewInv() * vertices[3];
				//		mathVec4TransformF(&vf[0], &vertices[0], &GetCurrentViewInv());
				//		mathVec4TransformF(&vf[1], &vertices[1], &GetCurrentViewInv());
				//		mathVec4TransformF(&vf[2], &vertices[2], &GetCurrentViewInv());
				//		mathVec4TransformF(&vf[3], &vertices[3], &GetCurrentViewInv());
					}
				}
			}
			else // orthogonal projected geometry
			{
				// compute depth corrected thickness of line end points
				float thicknessV0(0.5f * thickness * ComputeConstantScale(v[0], GetCurrentView(), GetCurrentProj(), wndXRes_));
				float thicknessV1(0.5f * thickness * ComputeConstantScale(v[1], GetCurrentView(), GetCurrentProj(), wndXRes_));

				// compute line delta
				Vec2f delta(v[1].xy() - v[0].xy());

				// create normal of line delta
				Vec2f normalVec(-delta.y, delta.x);
				normalVec.normalize();;

				Vec2f n[2];
				n[0] = normalVec * thicknessV0 * 2.0f;
				n[1] = normalVec * thicknessV1 * 2.0f;

				// compute final world space vertices of thick line
				vf[0] = Vec4f(v[0].x + n[0].x, v[0].y + n[0].y, v[0].z, 1.0f);
				vf[1] = Vec4f(v[1].x + n[1].x, v[1].y + n[1].y, v[1].z, 1.0f);
				vf[2] = Vec4f(v[1].x - n[1].x, v[1].y - n[1].y, v[1].z, 1.0f);
				vf[3] = Vec4f(v[0].x - n[0].x, v[0].y - n[0].y, v[0].z, 1.0f);
			}

			XAuxVertex* pVertices(const_cast<XAuxVertex*>(&auxVertexBuffer[offset]));
			if (false == skipLine)
			{
				// copy data to vertex buffer
				pVertices[0].pos = vf[0].xyz();
				pVertices[0].color = col[0];
				pVertices[1].pos = vf[1].xyz();
				pVertices[1].color = col[1];
				pVertices[2].pos = vf[2].xyz();
				pVertices[2].color = col[1];
				pVertices[3].pos = vf[0].xyz();
				pVertices[3].color = col[0];
				pVertices[4].pos = vf[2].xyz();
				pVertices[4].color = col[1];
				pVertices[5].pos = vf[3].xyz();
				pVertices[5].color = col[0];
			}
			else
			{
				// invalidate parameter data of thick line stored in vertex buffer 
				// (generates two black degenerated triangles at (0,0,0))
				memset(pVertices, 0, sizeof(XAuxVertex)* 6);
			}
		}
	}

}

// ----------------------------------------------------------------

void XRenderAuxImp::PrepareRendering()
{
	// update transformation matrices
	matrices_.UpdateMatrices(renderer_);
	
	// get current window resultion and update aspect ratios
	wndXRes_ = renderer_.getWidth();
	wndYRes_ = renderer_.getHeight();

	aspect_ = 1.0f;
	aspectInv_ = 1.0f;
	if (wndXRes_ > 0 && wndYRes_ > 0)
	{
		aspect_ = (float)wndXRes_ / (float)wndYRes_;
		aspectInv_ = 1.0f / aspect_;
	}

	// reset DrawInFront mode
	curDrawInFrontMode_ = AuxGeom_DrawInFrontMode::DrawInFrontOff;

	// reset stream buffer manager
	auxGeomSBM_.Reset();

	// reset current VB/IB
	curVB_ = VidMemManager::null_id;
	curIB_ = VidMemManager::null_id;

	// reset current prim type
	curPrimType_ = XRenderAux::PrimType::Invalid;
}


bool XRenderAuxImp::SetShader(const XAuxGeomRenderFlags& renderFlags)
{
	if (pAuxGeomShader_ == nullptr)
	{
		pAuxGeomShader_ = renderer_.ShaderMan_.forName("AuxGeom");
	}

#if 1
	if (pAuxGeomShader_)
	{
		XRenderAux::PrimType::Enum newPrimType(XRenderAux::GetPrimType(renderFlags));

		uint32 passes;

		if (XRenderAux::PrimType::Obj != newPrimType)
		{
			core::StrHash techName("AuxGeometry");
			
			if (!pAuxGeomShader_->FXSetTechnique(techName)) {
				return false;
			}
			if (!pAuxGeomShader_->FXBegin(&passes, 0)) {
				return false;
			}
			if (!pAuxGeomShader_->FXBeginPass(0)) {
				return false;
			}

			// override the shaders states.
			// adjust render states based on current render flags
			AdjustRenderStates(renderFlags);


			core::StrHash name("matViewProj");
			Matrix44f matViewProjT;
			//	matViewProjT = matrices_.pCurTransMat->transposed();
			matViewProjT = *matrices_.pCurTransMat;
			pAuxGeomShader_->FXSetVSFloat(name, reinterpret_cast<Vec4f*>(&matViewProjT), 4);

			renderer_.FX_ComitParams();
		}
		else
		{
			
			core::StrHash techName("AuxGeometryObj");
			if (!pAuxGeomShader_->FXSetTechnique(techName)) {
				return false;
			}
			if (!pAuxGeomShader_->FXBegin(&passes, 0)) {
				return false;
			}
			if (!pAuxGeomShader_->FXBeginPass(0)) {
				return false;
			}

			AdjustRenderStates(renderFlags);

			core::StrHash name("matViewProj");
			Matrix44f matViewProjT;
			matViewProjT = *matrices_.pCurTransMat;
			pAuxGeomShader_->FXSetVSFloat(name, reinterpret_cast<Vec4f*>(&matViewProjT), 4);
		}


		curDrawInFrontMode_ = AuxGeom_DrawInFrontMode::DrawInFrontOff;
		curPrimType_ = newPrimType;
	}
#endif

	return true;
}


void XRenderAuxImp::AdjustRenderStates(const XAuxGeomRenderFlags& renderFlags)
{
	// init current render states mask
	StateFlag state;

	// mode 2D/3D -- set new transformation matrix
	const Matrix44f* pNewTransMat(&GetCurrentTrans3D());

	if (AuxGeom_Mode2D3D::Mode2D == renderFlags.GetMode2D3DFlag())
	{
		pNewTransMat = &GetCurrentTrans2D();
	}
	if (matrices_.pCurTransMat != pNewTransMat)
	{
		matrices_.pCurTransMat = pNewTransMat;

	}

	// set alpha blending mode
	switch (renderFlags.GetAlphaBlendMode())
	{
		case AuxGeom_AlphaBlendMode::AlphaAdditive:
		{
			state |= States::BLEND_SRC_ONE | States::BLEND_DEST_ONE;
			break;
		}
		case AuxGeom_AlphaBlendMode::AlphaBlended:
		{
			state |= States::BLEND_SRC_SRC_ALPHA | States::BLEND_DEST_INV_SRC_ALPHA;
			break;
		}
		case AuxGeom_AlphaBlendMode::AlphaNone:
		default:
		{
			break;
		}
	}

	// set fill mode
	switch (renderFlags.GetFillMode())
	{
		case AuxGeom_FillMode::FillModeWireframe:
		{
			state |= States::WIREFRAME;
			break;
		}
		case AuxGeom_FillMode::FillModeSolid:
		default:
		{
			break;
		}
	}

	// set cull mode
	switch (renderFlags.GetCullMode())
	{
		case AuxGeom_CullMode::CullModeNone:
		{
			renderer_.SetCullMode(CullMode::NONE);
			break;
		}
		case AuxGeom_CullMode::CullModeFront:
		{
			renderer_.SetCullMode(CullMode::FRONT);
			break;
		}
		case AuxGeom_CullMode::CullModeBack:
		default:
		{
			renderer_.SetCullMode(CullMode::BACK);
			break;
		}
	}

	// set depth write mode
	switch (renderFlags.GetDepthWriteFlag())
	{
		case AuxGeom_DepthWrite::DepthWriteOff:
		{
			state = (state.ToInt() & ~States::DEPTHWRITE);
			break;
		}
		case AuxGeom_DepthWrite::DepthWriteOn:
		default:
		{
			state |= States::DEPTHWRITE;
			break;
		}
	}

	// set depth test mode
	switch (renderFlags.GetDepthTestFlag())
	{
		case AuxGeom_DepthTest::DepthTestOff:
		{
			state |= States::NO_DEPTH_TEST;
			break;
		}
		case AuxGeom_DepthTest::DepthTestOn:
		default:
		{
			break;
		}
	}

	// set point size
	uint8 newPointSize = curPointSize_;
	if (XRenderAux::PrimType::PtList == XRenderAux::GetPrimType(renderFlags))
	{
		newPointSize = XRenderAux::GetPointSize(renderFlags);
	}
	else
	{
		newPointSize = 1;
	}

	if (newPointSize != curPointSize_)
	{
		X_ASSERT_UNREACHABLE();
	}


	// apply states 
	renderer_.SetState(state);
}

bool XRenderAuxImp::BindStreams(shader::VertexFormat::Enum newVertexFormat,
	uint32_t NewVB, uint32_t NewIB)
{
	// set vertex declaration
	if (!renderer_.FX_SetVertexDeclaration(newVertexFormat, false))
		return false;

	// bind streams

	if (curVB_ != NewVB)
	{
		renderer_.FX_SetVStream(NewVB, VertexStream::VERT, XRender::vertexFormatStride[newVertexFormat], 0);
		curVB_ = NewVB;
	}
	if (curIB_ != NewIB)
	{
		renderer_.FX_SetIStream(NewIB);
		curIB_ = NewIB;
	}

	return true;
}


// ----------------------------------------------------------------

const Matrix44f& XRenderAuxImp::GetCurrentView() const
{
	return IsOrthoMode() ? identiy_mat : matrices_.matView;
}

const Matrix44f& XRenderAuxImp::GetCurrentViewInv() const
{
	return IsOrthoMode() ? identiy_mat : matrices_.matViewInv;
}

const Matrix44f& XRenderAuxImp::GetCurrentProj() const
{
	return IsOrthoMode() ? GetAuxOrthoMatrix(curTransMatrixIdx_) : matrices_.matProj;

}

const Matrix44f& XRenderAuxImp::GetCurrentTrans3D() const
{
	return IsOrthoMode() ? GetAuxOrthoMatrix(curTransMatrixIdx_) : matrices_.matTrans3D;
}

const Matrix44f& XRenderAuxImp::GetCurrentTrans2D() const
{
	return matrices_.matTrans2D;
}

bool XRenderAuxImp::IsOrthoMode() const
{
	return curTransMatrixIdx_ != -1;
}

void XRenderAuxImp::XMatrices::UpdateMatrices(DX11XRender& renderer)
{
	renderer.GetModelViewMatrix(&matView);
	renderer.GetProjectionMatrix(&matProj);

	matViewInv = matView.inverted();
	matTrans3D = matProj * matView;

	pCurTransMat = nullptr;
}



// ----------------------------------------------------------------

XRenderAuxImp::XStreamBufferManager::XStreamBufferManager() :
discardVB(true),
discardIB(true),
curVBIndex(0),
curIBIndex(0)
{

}

void XRenderAuxImp::XStreamBufferManager::Reset()
{
	discardVB = true;
	discardIB = true;
	curVBIndex = 0;
	curIBIndex = 0;
}

void XRenderAuxImp::XStreamBufferManager::DiscardVB()
{
	discardVB = true;
	curVBIndex = 0;
}

void XRenderAuxImp::XStreamBufferManager::DiscardIB()
{
	discardIB = true;
	curIBIndex = 0;
}
// ----------------------------------------------------------------

const XRenderAux::AuxVertexBuffer& XRenderAuxImp::GetAuxVertexBuffer() const
{
	X_ASSERT_NOT_NULL(pCurCBRawData_);
	return pCurCBRawData_->auxVertexBuffer;
}

const XRenderAux::AuxIndexBuffer& XRenderAuxImp::GetAuxIndexBuffer() const
{
	X_ASSERT_NOT_NULL(pCurCBRawData_);
	return pCurCBRawData_->auxIndexBuffer;
}

const XRenderAux::AuxDrawObjParamBuffer& XRenderAuxImp::GetAuxDrawObjParamBuffer() const
{
	X_ASSERT_NOT_NULL(pCurCBRawData_);
	return pCurCBRawData_->auxDrawObjParamBuffer;
}

const Matrix44f& XRenderAuxImp::GetAuxOrthoMatrix(int idx) const
{
	X_ASSERT_NOT_NULL(pCurCBRawData_);
	X_ASSERT(idx >= 0 && idx < (int)pCurCBRawData_->auxOrthoMatrices.size(), "index out of range")(idx);

	return pCurCBRawData_->auxOrthoMatrices[idx];
}



X_NAMESPACE_END