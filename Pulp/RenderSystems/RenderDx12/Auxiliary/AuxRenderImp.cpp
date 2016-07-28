#include "stdafx.h"
#include "AuxRenderImp.h"


X_NAMESPACE_BEGIN(render)


void AuxObjMesh::release(void)
{
	X_ASSERT_NOT_IMPLEMENTED();
}


// -----------------------------------------------------

const int32_t RenderAuxImp::AUX_GEOM_VBSIZE = 32768;
const int32_t RenderAuxImp::AUX_GEOM_IBSIZE = AUX_GEOM_IBSIZE * 2;
const float32_t RenderAuxImp::CLIP_THRESHOLD = .1f;


RenderAuxImp::RenderAuxImp(core::MemoryArenaBase* arena) :
	arena_(arena),
	auxSortedPushArr_(arena)
{

}

RenderAuxImp::~RenderAuxImp()
{

}



bool RenderAuxImp::init(void)
{
	if (!createLods(arena_)) {
		X_ERROR("Dx12", "Failed to create Aux object lods");
		return false;
	}

	return true;
}


void RenderAuxImp::shutDown(void)
{
	releaseLods();

}

void RenderAuxImp::flush(const AuxGeomCBRawDataPackaged& data, size_t begin, size_t end)
{
	X_ASSERT_NOT_NULL(data.pData_);
	pCurCBRawData_ = data.pData_;


	pCurCBRawData_->getSortedPushBuffer(begin, end, auxSortedPushArr_);

	auto itEnd = auxSortedPushArr_.end();
	for (auto it = auxSortedPushArr_.begin(); it != itEnd; ++it)
	{
		auto itCur = it;

		const XAuxGeomRenderFlags& curRenderFlags = (*itCur)->renderFlags;
		const auto primType = RenderAux::getPrimType(curRenderFlags);

		// find all entries sharing the same render flags
		X_DISABLE_WARNING(4127)
		while (true)
		{
			++it;
			if ((it == itEnd) || ((*it)->renderFlags != curRenderFlags)) {
				break;
			}
		}
		X_ENABLE_WARNING(4127)

		// prepare thick lines
		if (RenderAux::PrimType::TriList == primType && RenderAux::isThickLine(curRenderFlags))
		{
			if (AuxGeom_Mode2D3D::Mode3D == curRenderFlags.GetMode2D3DFlag())
			{
				prepareThickLines3D(itCur, it);
			}
			else
			{
				prepareThickLines2D(itCur, it);
			}
		}

		// draw push buffer entries
		switch (primType)
		{
		case RenderAux::PrimType::PointList:
		case RenderAux::PrimType::LineList:
		case RenderAux::PrimType::TriList:	
			drawAuxPrimitives(itCur, it, primType);
			break;	
		case RenderAux::PrimType::LineListInd:
		case RenderAux::PrimType::TriListInd:	
			drawAuxIndexedPrimitives(itCur, it, primType);
			break;	
		case RenderAux::PrimType::Obj:
		default:	
			drawAuxObjects(itCur, it);
			break;
		}

	}
	pCurCBRawData_ = nullptr;

}


void RenderAuxImp::determineAuxPrimitveFlags(uint32& d3dNumPrimDivider, PrimitiveType::Enum& d3dPrim,
	RenderAux::PrimType::Enum primType) const
{
	switch (primType)
	{
	case RenderAux::PrimType::PointList:
		d3dNumPrimDivider = 1;
		d3dPrim = PrimitiveType::PointList;
		break;
	case RenderAux::PrimType::LineList:
	case RenderAux::PrimType::LineListInd:
		d3dNumPrimDivider = 2;
		d3dPrim = PrimitiveType::LineList;
		break;	
	case RenderAux::PrimType::TriList:
	case RenderAux::PrimType::TriListInd:
	default:
		d3dNumPrimDivider = 3;
		d3dPrim = PrimitiveType::TriangleList;
		break;
	
	}
}

void RenderAuxImp::drawAuxPrimitives(RenderAux::AuxSortedPushArr::ConstIterator itBegin,
	RenderAux::AuxSortedPushArr::ConstIterator itEnd,
	const RenderAux::PrimType::Enum primType)
{

}

void RenderAuxImp::drawAuxIndexedPrimitives(RenderAux::AuxSortedPushArr::ConstIterator itBegin,
	RenderAux::AuxSortedPushArr::ConstIterator itEnd,
	const RenderAux::PrimType::Enum primType)
{

}

void RenderAuxImp::drawAuxObjects(RenderAux::AuxSortedPushArr::ConstIterator itBegin,
	RenderAux::AuxSortedPushArr::ConstIterator itEnd)
{

}

void RenderAuxImp::prepareThickLines2D(RenderAux::AuxSortedPushArr::ConstIterator itBegin,
	RenderAux::AuxSortedPushArr::ConstIterator itEnd)
{
	const RenderAux::AuxVertexArr& auxVertexBuffer = getAuxVertexBuffer();

	// process each entry
	for (auto it = itBegin; it != itEnd; ++it)
	{
		// get current push buffer entry
		const RenderAux::AuxPushBufferEntry* curPBEntry = *it;

		uint32 offset = curPBEntry->vertexOffs;
		uint32 i;
		for (i = 0; i < curPBEntry->numVertices / 6; ++i, offset += 6)
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

void RenderAuxImp::prepareThickLines3D(RenderAux::AuxSortedPushArr::ConstIterator itBegin,
	RenderAux::AuxSortedPushArr::ConstIterator itEnd)
{
	const RenderAux::AuxVertexArr& auxVertexBuffer = getAuxVertexBuffer();

	// process each entry
	for (auto it = itBegin; it != itEnd; ++it)
	{
		// get current push buffer entry
		const RenderAux::AuxPushBufferEntry* curPBEntry = *it;

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

			if (!isOrthoMode()) // regular, 3d projected geometry
			{
				skipLine = !clipLine(v, col);
				if (false == skipLine)
				{
					// compute depth corrected thickness of line end points
					float32_t thicknessV0(0.5f * thickness * computeConstantScale(v[0], getCurrentView(), getCurrentProj(), wndXRes_));
					float32_t thicknessV1(0.5f * thickness * computeConstantScale(v[1], getCurrentView(), getCurrentProj(), wndXRes_));

					// compute camera space line delta
					Vec4f vt[2];

					vt[0] = getCurrentView() * v[0];
					vt[1] = getCurrentView() * v[1];

					if (vt[0].z != 0.0f && vt[1].z != 0.0f)
					{
						vt[0].z = fsel(-vt[0].z - CLIP_THRESHOLD, vt[0].z, -CLIP_THRESHOLD);
						vt[1].z = fsel(-vt[1].z - CLIP_THRESHOLD, vt[1].z, -CLIP_THRESHOLD);

						Vec2f delta(((vt[1] / vt[1].z) - (vt[0] / vt[0].z)).xy());

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


						vf[0] = getCurrentViewInv() * vertices[0];
						vf[1] = getCurrentViewInv() * vertices[1];
						vf[2] = getCurrentViewInv() * vertices[2];
						vf[3] = getCurrentViewInv() * vertices[3];
					}
				}
			}
			else // orthogonal projected geometry
			{
				// compute depth corrected thickness of line end points
				float32_t thicknessV0(0.5f * thickness * computeConstantScale(v[0], getCurrentView(), getCurrentProj(), wndXRes_));
				float32_t thicknessV1(0.5f * thickness * computeConstantScale(v[1], getCurrentView(), getCurrentProj(), wndXRes_));

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
				memset(pVertices, 0, sizeof(XAuxVertex) * 6);
			}
		}
	}
}


bool RenderAuxImp::clipLine(Vec3f v[2], Color8u c[2])
{
	// get clipping flags
	const bool bV0Behind = (-(nearPlane_.getNormal().dot(v[0]) + nearPlane_.getDistance()) < CLIP_THRESHOLD);
	const bool bV1Behind = (-(nearPlane_.getNormal().dot(v[1]) + nearPlane_.getDistance()) < CLIP_THRESHOLD);

	// proceed only if both are not behind near clipping plane
	if (!bV0Behind || !bV1Behind)
	{
		if (!bV0Behind && !bV1Behind)
		{
			// no clipping needed
			return true;
		}

		// define line to be clipped
		Vec3f p(v[0]);
		Vec3f d(v[1] - v[0]);

		// get clipped position
		float32_t t = 0.f;
		v[0] = (!bV0Behind) ? v[0] : intersectLinePlane(p, d, nearPlane_, t);
		v[1] = (!bV1Behind) ? v[1] : intersectLinePlane(p, d, nearPlane_, t);

		// get clipped colors
		c[0] = (!bV0Behind) ? c[0] : clipColor(c[0], c[1], t);
		c[1] = (!bV1Behind) ? c[1] : clipColor(c[0], c[1], t);

		return true;
	}

	return false;
}


Vec3f RenderAuxImp::intersectLinePlane(const Vec3f& o, const Vec3f& d,
	const Planef& p, float& t)
{
	t = -(p.getNormal().dot(o) + (p.getDistance() + CLIP_THRESHOLD)) / p.getNormal().dot(d);
	return (o + d * t);
}


Color8u RenderAuxImp::clipColor(const Color8u& c0, const Color8u& c1, float32_t t)
{
	Color8u v0(c0);
	return (c0 + (c1 - v0) * t);
}


float32_t RenderAuxImp::computeConstantScale(const Vec3f& v, const Matrix44f& matView,
	const Matrix44f& matProj, const uint32 wndXRes)
{
	Vec4f vCam0;

	vCam0 = matView * v;

	Vec4f vCam1(vCam0);
	vCam1.x += 1.0f;

	float32_t d0(vCam0.x * matProj.m03 +
		vCam0.y * matProj.m13 +
		vCam0.z * matProj.m23 +
		matProj.m33);

	if (d0 == 0.0f) {
		d0 = FLT_EPSILON;
	}

	float32_t c0((vCam0.x * matProj.m00 +
		vCam0.y * matProj.m10 +
		vCam0.z * matProj.m20 +
		matProj.m30) / d0);

	float32_t d1(vCam1.x * matProj.m03 +
		vCam1.y * matProj.m13 +
		vCam1.z * matProj.m23 +
		matProj.m33);

	if (d1 == 0.0f) {
		d1 = FLT_EPSILON;
	}

	float32_t c1((vCam1.x * matProj.m00 +
		vCam1.y * matProj.m10 +
		vCam1.z * matProj.m20 +
		matProj.m30) / d1);

	const float32_t epsilon = 0.001f;
	float32_t s = static_cast<float32_t>(wndXRes) * (c1 - c0);
	return (math<float>::abs(s) >= epsilon) ? 1.0f / s : 1.0f / epsilon;
}


X_NAMESPACE_END