#pragma once

#include <Math\VertexFormats.h>
#include "Buffers\GpuBuffer.h"

X_NAMESPACE_BEGIN(render)


class Mesh
{
public:
	Mesh();
	~Mesh();


private:
	shader::VertexFormat::Enum vertexFmt_;

	StructuredBuffer vertexStreams_[VertexStream::ENUM_COUNT];
	ByteAddressBuffer indexStream_;
};


X_NAMESPACE_END