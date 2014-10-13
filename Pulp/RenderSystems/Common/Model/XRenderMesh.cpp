#include "stdafx.h"
#include "XRenderMesh.h"

X_NAMESPACE_BEGIN(model)



XRenderMesh::XRenderMesh(model::LODHeader* pLod, const char* pName) 
{
	X_ASSERT_NOT_NULL(pLod);
	X_ASSERT_NOT_NULL(pName);

	name_ = pName;
	pLod_ = pLod;
}

XRenderMesh::~XRenderMesh()
{

}

// returns false if no Video memory.
bool XRenderMesh::canRender(void)
{

	return false;
}

// genral Info
const char* XRenderMesh::getName(void) const
{
	return name_.c_str();
}

int XRenderMesh::getNumVerts(void) const
{
	return pLod_->numVerts;
}

int XRenderMesh::getNumIndexs(void) const
{
	return pLod_->numIndexs;
}

int XRenderMesh::getNumSubMesh(void) const
{
	return pLod_->numMesh;
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