#pragma once

#ifndef X_RENDER_MODEL_H_
#define X_RENDER_MODEL_H_

#include <Containers\Array.h>

#include <Assets\AssertContainer.h>
#include <Math\VertexFormats.h>
#include <Util\ReferenceCounted.h>

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
    XMeshDevBuf();

    bool isValid(void) const;

    //	void* pDevBuf;
    uint32_t BufId;
    uint32_t stride;
};

struct XMeshSurface
{
};

// A mesh can be a model or world geo.
// they are both treated the same for rendering.
// this class supports sub mehses but all of them are stored
// in a single vertex / index buffer
// since it's pretty rare a mesh dose not render all it's sub meshes.
//
// Supports multiple vertex formats.
// Should I allow support for 32bit indices.
// Only need for it would be for map's that have like no portals.
//
//	I'm leaning towards, the stance that if you want more than 65k verts.
//	fucking portal your map lol.
//
//	65k verts allows for: (65k / 6): 10922 square faces
//
//	That is without index duplicate removing, which should allow for a very large amount, since 95% of brush
//	sides will use the same 6 indexs.
//
//
//
//
//
class XRenderMesh : public IRenderMesh
    , public core::ReferenceCounted<XRenderMesh>
{
public:
    XRenderMesh();
    XRenderMesh(const model::MeshHeader* pMesh, shader::VertexFormat::Enum fmt, const char* name);
    ~XRenderMesh() X_OVERRIDE;

    // IRenderModel

    virtual const int addRef(void) X_OVERRIDE
    {
        return this->addReference();
    }
    virtual const int release(void) X_OVERRIDE
    {
        const int refs = this->removeReference();
        if (refs == 0) {
            X_DELETE(this, g_rendererArena);
        }
        return refs;
    }

    // draw
    virtual bool render(void) X_OVERRIDE;

    // returns false if no Video memory.
    virtual bool canRender(void) X_OVERRIDE;
    virtual bool uploadToGpu(void) X_OVERRIDE;

    // genral Info
    virtual const char* getName(void) const X_OVERRIDE;
    virtual int getNumVerts(void) const X_OVERRIDE;
    virtual int getNumIndexes(void) const X_OVERRIDE;
    virtual int getNumSubMesh(void) const X_OVERRIDE;

    virtual shader::VertexFormat::Enum getVertexFmt(void) const X_OVERRIDE
    {
        return vertexFmt_;
    }

    // Mem Info
    virtual int MemoryUsageTotal(void) const X_OVERRIDE;
    virtual int MemoryUsageSys(void) const X_OVERRIDE;
    virtual int MemoryUsageVideo(void) const X_OVERRIDE;
    // ~IRenderModel

    X_INLINE bool hasVBStream(VertexStream::Enum type) const
    {
        return vertexStreams_[type].isValid();
    }
    X_INLINE bool hasIBStream(void) const
    {
        return indexStream_.isValid();
    }

    void freeVB(VertexStream::Enum stream); // VertexBuffer
    void freeIB(void);                      // Index Buffer
    void freeVideoMem(bool restoreSys = false);
    void freeSystemMem(void);

protected:
    core::string name_;

    shader::VertexFormat::Enum vertexFmt_;

    model::MeshHeader const* pMesh_;

    // we can have multiple streams.
    XMeshDevBuf vertexStreams_[VertexStream::ENUM_COUNT];
    XMeshDevBuf indexStream_;
};

X_NAMESPACE_END

#endif // !X_RENDER_MODEL_H_