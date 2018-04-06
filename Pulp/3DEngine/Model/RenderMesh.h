#pragma once

#include <IRender.h>

X_NAMESPACE_BEGIN(model)

// just a simple container for the buffer handles.

class XRenderMesh
{
public:
    struct SizeInfo
    {
        SizeInfo();

        size_t size;
        size_t deviceSize;
    };

    typedef render::shader::VertexFormat VertexFormat;

public:
    XRenderMesh();
    XRenderMesh(const XRenderMesh& oth) = default;
    ~XRenderMesh() = default;

    bool canRender(void) const;

    bool createRenderBuffers(render::IRender* pRend, const MeshHeader& mesh, VertexFormat::Enum vertexFmt);
    bool createSkinningRenderBuffers(render::IRender* pRend, const MeshHeader& mesh);

    void releaseRenderBuffers(render::IRender* pRend);

    // creates the device buffer for a given stream if don't already exsist
    bool ensureRenderStream(render::IRender* pRend, const MeshHeader& mesh, VertexFormat::Enum vertexFmt, VertexStream::Enum stream);

    // release a single stream, used to free up vram if a stream is unlikley to be used soon.
    void releaseVertexBuffer(render::IRender* pRend, VertexStream::Enum stream);
    void releaseIndexBuffer(render::IRender* pRend);

    SizeInfo memoryUsage(render::IRender* pRend) const;

    X_INLINE bool hasVBStream(VertexStream::Enum type) const;
    X_INLINE bool hasIBStream(void) const;

    X_INLINE const render::VertexBufferHandleArr& getVBBuffers(void) const;
    X_INLINE render::IndexBufferHandle getIBBuffer(void) const;

private:
    render::VertexBufferHandleArr vertexStreams_;
    render::IndexBufferHandle indexStream_;
};

X_NAMESPACE_END

#include "RenderMesh.inl"