#include "stdafx.h"
#include "XRenderMesh.h"

#include "Dx10Render.h"

#include "DeviceManager\VidMemManager.h"

X_NAMESPACE_BEGIN(model)


XRenderMesh::XRenderMesh()
{
	pMesh_ = nullptr;
	vertexFmt_ = (shader::VertexFormat::Enum) - 1;

	indexStream_.BufId = render::VidMemManager::null_id;
}

XRenderMesh::XRenderMesh(model::MeshHeader* pMesh, shader::VertexFormat::Enum fmt, 
	const char* pName)
{
	X_ASSERT_NOT_NULL(pMesh);
	X_ASSERT_NOT_NULL(pName);

	name_ = pName;
	pMesh_ = pMesh;
	vertexFmt_ = fmt;

	indexStream_.BufId = render::VidMemManager::null_id;
}

XRenderMesh::~XRenderMesh()
{
}

// returns false if no Video memory.
bool XRenderMesh::canRender(void)
{
	using namespace render;

	return indexStream_.BufId != VidMemManager::null_id && 
		vertexStreams_[VertexStream::VERT].BufId != VidMemManager::null_id;
}

bool XRenderMesh::uploadToGpu(void)
{
	X_ASSERT_NOT_NULL(pMesh_);
	X_ASSERT((int32)vertexFmt_ != -1, "vertex format has not been set")(vertexFmt_);
	using namespace render;

	uint32_t ibSize, vbSize;

	ibSize = pMesh_->numIndexes * sizeof(model::Index);
	vbSize = pMesh_->numVerts * DX11XRender::vertexFormatStride[vertexFmt_];

	indexStream_.BufId = g_Dx11D3D.VidMemMng()->CreateIB(ibSize, pMesh_->indexes);
	vertexStreams_[VertexStream::VERT].BufId = g_Dx11D3D.VidMemMng()->CreateVB(vbSize, pMesh_->verts);

	return canRender();
}


bool XRenderMesh::render(void)
{
	using namespace render;

	if (!canRender())
		return false;

	Matrix44f WPM;
	Matrix44f View;
	Matrix44f Projection;


	g_Dx11D3D.SetWorldShader();
	g_Dx11D3D.FX_SetVertexDeclaration(vertexFmt_);
	g_Dx11D3D.FX_SetIStream(indexStream_.BufId);
	g_Dx11D3D.FX_SetVStream(
		vertexStreams_[VertexStream::VERT].BufId, 
		0, 
		DX11XRender::vertexFormatStride[vertexFmt_],
		0
	);

	uint32_t i, num;
	
	num = pMesh_->numSubMeshes;

	for (i = 0; i < num; i++)
	{
		const model::SubMeshHeader* mesh = pMesh_->subMeshHeads[i];

		g_Dx11D3D.FX_DrawIndexPrimitive(
			PrimitiveType::TriangleList,
			mesh->numIndexes,
			mesh->startIndex,
			mesh->startVertex
			);

	}
//	g_Dx11D3D.Set2D(false);

	return true;
}

// genral Info
const char* XRenderMesh::getName(void) const
{
	return name_.c_str();
}

int XRenderMesh::getNumVerts(void) const
{
	return pMesh_->numVerts;
}

int XRenderMesh::getNumIndexes(void) const
{
	return pMesh_->numIndexes;
}

int XRenderMesh::getNumSubMesh(void) const
{
	return pMesh_->numSubMeshes;
}



// Mem Info
int XRenderMesh::MemoryUsageTotal(void) const
{
	return MemoryUsageSys() + MemoryUsageVideo();
}

int XRenderMesh::MemoryUsageSys(void) const
{
	return 0;
}

int XRenderMesh::MemoryUsageVideo(void) const
{

	return 0;
}



void XRenderMesh::freeVB(VertexStream::Enum stream) // VertexBuffer
{
	if (vertexStreams_[stream].isValid())
	{


	}
}

void XRenderMesh::freeIB(void) // Index Buffer
{

}

void XRenderMesh::freeVideoMem(bool restoreSys)
{

}

void XRenderMesh::freeSystemMem(void)
{

}


X_NAMESPACE_END