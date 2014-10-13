#pragma once

#ifndef X_RENDER_MODEL_H_
#define X_RENDER_MODEL_H_

#include <Containers\Array.h>

#include <Assets\AssertContainer.h>
#include <Math\VertexFormats.h>

#include <IRenderMesh.h>


X_NAMESPACE_BEGIN(model)

//
//	Note: a model with 4 LOD's is 4 meshes not 1.
// 
//	This is a class for storeing a single mesh / LOD
//
//  this class dose not own the system memory.
//
//	we store all the LOD's meshes in a contigous vertex / index buffer
//  since it's very rare that we don't want to draw part of a mesh
//	(hide tag only case i can thing of for now.)
//
//  meaning we only have to bind one VB / IB to render the whole LOD.
//
//  we store offset info so each sub mesh can be drawn seprately with
//  a diffrent material.
//
//
//  we have a diffrent stream for bind data.
//
//

struct XMeshDevBuf
{
	XMeshDevBuf() : pDevBuf(nullptr), BufId(0) {}

	X_INLINE bool isValid(void) const {
		return pDevBuf != nullptr;
	}

	void* pDevBuf;
	uint32_t BufId;
};

struct XMeshDevStream
{
	XMeshDevStream() {
	}

	X_INLINE bool isValid(void) const {
		return true;
	}


	XMeshDevBuf devBuf;    // device buffer
};



// A mesh can be a model or world geo.
// they are both treated the same for rendering.
// this class supports sub mehses but all of them are stored
// in a single vertex / index buffer
class XRenderMesh : public IRenderMesh, public core::XBaseAsset
{
public:
	XRenderMesh(model::LODHeader* pLod, const char* name);
	~XRenderMesh() X_OVERRIDE;

	// IRenderModel

	virtual const int addRef(void) X_OVERRIDE{
		return XBaseAsset::addRef();
	}
	virtual const int release(void) X_OVERRIDE{
		const int refs = XBaseAsset::release();
		if (refs == 0) {


		}
		return refs;
	}

	// returns false if no Video memory.
	virtual bool canRender(void) X_OVERRIDE;

	// genral Info
	virtual const char* getName(void) const X_OVERRIDE;
	virtual int getNumVerts(void) const X_OVERRIDE;
	virtual int getNumIndexs(void) const X_OVERRIDE;
	virtual int getNumSubMesh(void) const X_OVERRIDE;

	virtual shader::VertexFormat::Enum getVertexFmt(void) const X_OVERRIDE{
		return vertexFmt_;
	}

	// Mem Info
	virtual int MemoryUsageTotal(void) const X_OVERRIDE;
	virtual int MemoryUsageSys(void) const X_OVERRIDE;
	virtual int MemoryUsageVideo(void) const X_OVERRIDE;
	// ~IRenderModel
	
	X_INLINE bool hasVBStream(VertexStream::Enum type) const { return vertexStreams_[type].isValid(); }
	X_INLINE bool hasIBStream(void) const { return indexStream_.isValid(); }

	void freeVB(VertexStream::Enum stream); // VertexBuffer
	void freeIB(void); // Index Buffer
	void freeVideoMem(bool restoreSys = false);
	void freeSystemMem(void);

protected:
	// might make a const char* since lod is owned.
	// or if i make model have a string, memory will just be ref counted.
	core::string name_; 

	shader::VertexFormat::Enum vertexFmt_;

	model::LODHeader const* pLod_;

	// we can have multiple streams.
	XMeshDevStream vertexStreams_[VertexStream::ENUM_COUNT];
	XMeshDevStream indexStream_;
};




X_NAMESPACE_END

#endif // !X_RENDER_MODEL_H_