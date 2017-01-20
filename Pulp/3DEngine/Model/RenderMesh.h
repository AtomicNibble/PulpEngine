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

public:
	XRenderMesh() = default;
	~XRenderMesh() = default;

	bool canRender(void) const;

	bool createRenderBuffers(render::IRender* pRend, const MeshHeader& mesh, render::shader::VertexFormat::Enum vertexFmt);
	void releaseRenderBuffers(render::IRender* pRend);

	SizeInfo memoryUsage(render::IRender* pRend) const;

	X_INLINE bool hasVBStream(VertexStream::Enum type) const;
	X_INLINE bool hasIBStream(void) const;

private:
	render::VertexBufferHandle vertexStreams_[VertexStream::ENUM_COUNT];
	render::IndexBufferHandle indexStream_;
};


X_NAMESPACE_END