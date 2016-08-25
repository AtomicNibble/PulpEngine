

X_NAMESPACE_BEGIN(model)


bool XRenderMesh::hasVBStream(VertexStream::Enum type) const
{
	return vertexStreams_[type] != render::INVALID_BUF_HANLDE;
}

bool XRenderMesh::hasIBStream(void) const
{
	return indexStream_ != render::INVALID_BUF_HANLDE;
}


X_NAMESPACE_END