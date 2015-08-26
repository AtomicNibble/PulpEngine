#include "stdafx.h"
#include "Dx10RenderAux.h"

#include "DeviceManager\VidMemManager.h"

X_NAMESPACE_BEGIN(render)

namespace 
{

	typedef std::vector< XAuxObjVertex > AuxObjVertexBuffer;
	typedef std::vector< uint16 > AuxObjIndexBuffer;

	// function to generate a sphere mesh
	static void CreateSphere(AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib,
		float radius, uint32_t rings, uint32_t sections)
	{
		// calc required number of vertices/indices/triangles to build a sphere for the given parameters
		uint32 numVertices, numTriangles, numIndices;

		numVertices = (rings - 1) * (sections + 1) + 2;
		numTriangles = (rings - 2) * sections * 2 + 2 * sections;
		numIndices =numTriangles * 3;

		// setup buffers
		vb.clear();
		vb.reserve(numVertices);

		ib.clear();
		ib.reserve(numIndices);

		// 1st pole vertex
		vb.push_back(XAuxObjVertex(Vec3f(0.0f, 0.0f, radius), Vec3f(0.0f, 0.0f, 1.0f)));

		// calculate "inner" vertices
		float sectionSlice = toRadians(360.0f / (float)sections);
		float ringSlice = toRadians(180.0f / (float)rings);

		uint32 a, i;
		for (a=1; a < rings; ++a)
		{
			float w = math<float>::sin(a * ringSlice);
			for (i=0; i <= sections; ++i)
			{
				Vec3f v;
				v.x = radius * cosf(i * sectionSlice) * w;
				v.y = radius * sinf(i * sectionSlice) * w;
				v.z = radius * cosf(a * ringSlice);
				vb.push_back(XAuxObjVertex(v, v.normalized()));
			}
		}

		// 2nd vertex of pole (for end cap)
		vb.push_back(XAuxObjVertex(Vec3f(0.0f, 0.0f, -radius), Vec3f(0.0f, 0.0f, 1.0f)));

		// build "inner" faces
		for (uint32 a(0); a < rings - 2; ++a)
		{
			for (uint32 i(0); i < sections; ++i)
			{
				ib.push_back((uint16)(1 + a * (sections + 1) + i + 1));
				ib.push_back((uint16)(1 + a * (sections + 1) + i));
				ib.push_back((uint16)(1 + (a + 1) * (sections + 1) + i + 1));

				ib.push_back((uint16)(1 + (a + 1) * (sections + 1) + i));
				ib.push_back((uint16)(1 + (a + 1) * (sections + 1) + i + 1));
				ib.push_back((uint16)(1 + a * (sections + 1) + i));
			}
		}

		// build faces for end caps (to connect "inner" vertices with poles)
		for (uint32 i(0); i < sections; ++i)
		{
			ib.push_back((uint16)(1 + (0) * (sections + 1) + i));
			ib.push_back((uint16)(1 + (0) * (sections + 1) + i + 1));
			ib.push_back((uint16)0);
		}

		for (uint32 i(0); i < sections; ++i)
		{
			ib.push_back((uint16)(1 + (rings - 2) * (sections + 1) + i + 1));
			ib.push_back((uint16)(1 + (rings - 2) * (sections + 1) + i));
			ib.push_back((uint16)((rings - 1) * (sections + 1) + 1));
		}
	}



	// function to generate a cone mesh
	static void CreateCone(AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib, 
		float radius, float height, uint32_t sections)
	{
		// calc required number of vertices/indices/triangles to build a cone for the given parameters
		uint32 numVertices, numTriangles, numIndices;
		uint16_t i;

		numVertices = 2 * (sections + 1) + 2;
		numTriangles = 2 * sections;
		numIndices = numTriangles * 3;


		// setup buffers
		vb.clear();
		vb.reserve(numVertices);

		ib.clear();
		ib.reserve(numIndices);

		// center vertex
		vb.push_back(XAuxObjVertex(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, -1.0f, 0.0f)));

		// create circle around it
		float sectionSlice = toRadians(360.0f / (float)sections);
		for (i = 0; i <= sections; ++i)
		{
			Vec3f v;
			v.x = radius * cosf(i * sectionSlice);
			v.y = 0.0f;
			v.z = radius * sinf(i * sectionSlice);
			vb.push_back(XAuxObjVertex(v, Vec3f(0.0f, -1.0f, 0.0f)));
		}

		// build faces for end cap 
		for (i = 0; i < sections; ++i)
		{
			ib.push_back(0);
			ib.push_back(1 + i);
			ib.push_back(1 + i + 1);
		}

		// top
		vb.push_back(XAuxObjVertex(Vec3f(0.0f, height, 0.0f), Vec3f(0.0f, 1.0f, 0.0f)));

		for (i = 0; i <= sections; ++i)
		{
			Vec3f v;
			v.x = radius * cosf(i * sectionSlice);
			v.y = 0.0f;
			v.z = radius * sinf(i * sectionSlice);

			Vec3f v1;
			v1.x = radius * cosf(i * sectionSlice + 0.01f);
			v1.y = 0.0f;
			v1.z = radius * sinf(i * sectionSlice + 0.01f);

			Vec3f d(v1 - v);
			Vec3f d1(Vec3f(0.0, height, 0.0f) - v);

			Vec3f n((d1.cross(d)).normalized());
			vb.push_back(XAuxObjVertex(v, n));
		}

		// build faces
		for (i = 0; i < sections; ++i)
		{
			ib.push_back(safe_static_cast<uint16_t,uint32_t>(sections + 2));
			ib.push_back(safe_static_cast<uint16_t, uint32_t>(sections + 3 + i + 1));
			ib.push_back(safe_static_cast<uint16_t, uint32_t>(sections + 3 + i));
		}
	}


	// function to generate a cylinder mesh
	static void CreateCylinder(AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib, 
		float radius, float height, uint32_t sections)
	{
		// calc required number of vertices/indices/triangles to build a cylinder for the given parameters
		uint32 numVertices, numTriangles, numIndices;

		numVertices = 4 * (sections + 1) + 2;
		numTriangles = 4 * sections;
		numIndices = numTriangles * 3;

		// setup buffers
		vb.clear();
		vb.reserve(numVertices);

		ib.clear();
		ib.reserve(numIndices);

		float sectionSlice = toRadians(360.0f / (float)sections);

		// bottom cap
		{
			// center bottom vertex
			vb.push_back(XAuxObjVertex(Vec3f(0.0f, -0.5f * height, 0.0f), Vec3f(0.0f, -1.0f, 0.0f)));

			// create circle around it
			uint16_t i;
			for (i=0; i <= sections; ++i)
			{
				Vec3f v;
				v.x = radius * math<float>::cos(i * sectionSlice);
				v.y = -0.5f * height;
				v.z = radius * math<float>::sin(i * sectionSlice);
				vb.push_back(XAuxObjVertex(v, Vec3f(0.0f, -1.0f, 0.0f)));
			}

			// build faces
			for (i=0; i < sections; ++i)
			{
				ib.push_back(0);
				ib.push_back(1 + i);
				ib.push_back(1 + i + 1);
			}
		}

		// side
		{
			uint16_t vIdx = safe_static_cast<uint16_t, size_t>(vb.size());

			uint32 i;
			for (i=0; i <= sections; ++i)
			{
				Vec3f v;
				v.x = radius * math<float>::cos(i * sectionSlice);
				v.y = -0.5f * height;
				v.z = radius * math<float>::sin(i * sectionSlice);

				Vec3f n(v.normalized());
				vb.push_back(XAuxObjVertex(v, n));
				vb.push_back(XAuxObjVertex(Vec3f(v.x, -v.y, v.z), n));
			}

			// build faces
			for (i = 0; i < sections; ++i, vIdx += 2)
			{
				ib.push_back(vIdx);
				ib.push_back(vIdx + 1);
				ib.push_back(vIdx + 2);

				ib.push_back(vIdx + 1);
				ib.push_back(vIdx + 3);
				ib.push_back(vIdx + 2);
			}
		}

		// top cap
		{
			uint16_t vIdx = safe_static_cast<uint16_t, size_t>(vb.size());

			// center top vertex
			vb.push_back(XAuxObjVertex(Vec3f(0.0f, 0.5f * height, 0.0f), Vec3f(0.0f, 1.0f, 0.0f)));

			// create circle around it
			uint16_t i;
			for (i=0; i <= sections; ++i)
			{
				Vec3f v;
				v.x = radius * math<float>::cos(i * sectionSlice);
				v.y = 0.5f * height;
				v.z = radius * math<float>::sin(i * sectionSlice);
				vb.push_back(XAuxObjVertex(v, Vec3f(0.0f, 1.0f, 0.0f)));
			}

			// build faces
			for (i=0; i < sections; ++i)
			{
				ib.push_back(vIdx);
				ib.push_back(vIdx + 1 + i + 1);
				ib.push_back(vIdx + 1 + i);
			}
		}
	}


	// Functor to generate a sphere mesh. To be used with generalized CreateMesh function.
	struct XSphereMeshCreateFunc
	{
		XSphereMeshCreateFunc(float radius, uint32_t rings, uint32_t sections) :
			radius_(radius),
			rings_(rings),
			sections_(sections)
		{
		}

		void CreateMesh(AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib)
		{
			CreateSphere(vb, ib, radius_, rings_, sections_);
		}

		float radius_;
		uint32_t rings_;
		uint32_t sections_;
	};


	// Functor to generate a cone mesh. To be used with generalized CreateMesh function.
	struct XConeMeshCreateFunc
	{
		XConeMeshCreateFunc(float radius, float height, uint32_t sections) :
			radius_(radius),
			height_(height),
			sections_(sections)
		{
		}

		void CreateMesh(AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib)
		{
			CreateCone(vb, ib, radius_, height_, sections_);
		}

		float radius_;
		float height_;
		uint32_t sections_;
	};


	// Functor to generate a cylinder mesh. To be used with generalized CreateMesh function.
	struct XCylinderMeshCreateFunc
	{
		XCylinderMeshCreateFunc(float radius, float height, uint32_t sections) :
			radius_(radius),
			height_(height),
			sections_(sections)
		{
		}

		void CreateMesh(AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib)
		{
			CreateCylinder(vb, ib, radius_, height_, sections_);
		}

		float radius_;
		float height_;
		uint32_t sections_;
	};


} // namespace




// Generalized CreateMesh function to create any mesh via passed-in functor.
// The functor needs to provide a CreateMesh function which accepts an 
// AuxObjVertexBuffer and AuxObjIndexBuffer to stored the resulting mesh.
template< typename TMeshFunc >
bool XRenderAuxImp::CreateMesh(XDrawObjMesh& mesh, TMeshFunc meshFunc)
{
	// create mesh
	uint32_t ibID, vbID;
	AuxObjVertexBuffer vb;
	AuxObjIndexBuffer ib;

	meshFunc.CreateMesh(vb, ib);

	if (vb.empty() || ib.empty())
	{
		X_ERROR("Aux", "failed to create Aux mesh: %i %i", vb.size(), ib.size());
		return false;
	}

	VidMemManager* pMem = renderer_.VidMemMng();

	vbID = pMem->CreateVB(safe_static_cast<uint32_t, size_t>(vb.size() * sizeof(XAuxObjVertex)), &vb[0]);
	ibID = pMem->CreateIB(safe_static_cast<uint32_t, size_t>(ib.size() * sizeof(uint16_t)), &ib[0]);

	if (vbID == VidMemManager::null_id ||
		ibID == VidMemManager::null_id)
	{
		return false;
	}

	// write mesh info
	mesh.VBid = vbID;
	mesh.IBid = ibID;
	mesh.numVertices = safe_static_cast<uint32, size_t>(vb.size());
	mesh.numFaces = safe_static_cast<uint32, size_t>(ib.size() / 3);

	return true;
}


HRESULT XRenderAuxImp::RestoreDeviceObjects()
{
	HRESULT hr = S_OK;

	// release
	ReleaseDeviceObjects();

	VidMemManager* pMem = renderer_.VidMemMng();

	auxGeomVB_ = pMem->CreateVB(AuxGeom_VBSize * sizeof(XAuxVertex), CpuAccess::WRITE);
	auxGeomIB_ = pMem->CreateIB(AuxGeom_IBSize * sizeof(uint16_t), CpuAccess::WRITE);

	if (auxGeomVB_ == VidMemManager::null_id ||
		auxGeomIB_ == VidMemManager::null_id)
	{
		return S_FALSE;
	}

	// recreate aux objects
	uint32 i;
	for (i=0; i < auxObjNumLOD; ++i)
	{
		if (!CreateMesh(sphereObj_[i], XSphereMeshCreateFunc(1.0f, 9 + 4 * i, 9 + 4 * i)))
		{
			return S_FALSE;
		}

		if (!CreateMesh(coneObj_[i], XConeMeshCreateFunc(1.0f, 1.0f, 10 + i * 6)))
		{
			return S_FALSE;
		}

		if (!CreateMesh(cylinderObj_[i], XCylinderMeshCreateFunc(1.0f, 1.0f, 10 + i * 6)))
		{
			return S_FALSE;
		}
	}

	return hr;
}



void XRenderAuxImp::ReleaseDeviceObjects()
{
	VidMemManager* pMem = renderer_.VidMemMng();
	pMem->freeVB(auxGeomVB_);
	pMem->freeIB(auxGeomIB_);


	core::SafeRelease(pAuxGeomShader_);

	uint32 i;
	for (i = 0; i < auxObjNumLOD; ++i)
	{
		sphereObj_[i].Release();
		coneObj_[i].Release();
		cylinderObj_[i].Release();
	}
}


X_NAMESPACE_END