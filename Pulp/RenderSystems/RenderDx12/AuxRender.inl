

X_NAMESPACE_BEGIN(render)


X_INLINE AuxDrawObjParams::AuxDrawObjParams(const Matrix34f& matWorld_, const Color8u& color_,
	float32_t size_, bool shaded_) :
	matWorld(matWorld_),
	color(color_),
	size(size_),
	shaded(shaded_)
{

}

// ---------------------------------------------------

X_INLINE RenderAux::AuxPushBufferEntry::AuxPushBufferEntry(uint32 numVertices, uint32 numIndices,
	uint32 vertexOffs, uint32 indexOffs, const XAuxGeomRenderFlags& renderFlags) :
	numVertices(numVertices),
	numIndices(numIndices),
	vertexOffs(vertexOffs),
	indexOffs(indexOffs),
	transMatrixIdx(transMatrixIdx),
	renderFlags(renderFlags)
{

}

X_INLINE RenderAux::AuxPushBufferEntry::AuxPushBufferEntry(uint32 drawParamOffs,
	const XAuxGeomRenderFlags& renderFlags) :
	numVertices(0),
	numIndices(0),
	vertexOffs(drawParamOffs),
	indexOffs(0),
	transMatrixIdx(transMatrixIdx),
	renderFlags(renderFlags)
{

}

X_INLINE bool RenderAux::AuxPushBufferEntry::getDrawParamOffs(uint32& drawParamOffs) const
{
	if (PrimType::Obj == getPrimType(renderFlags)) {
		drawParamOffs = vertexOffs;
		return true;
	}

	return false;
}

// ---------------------------------------------------


X_INLINE void RenderAux::setRenderFlags(const XAuxGeomRenderFlags& renderFlags)
{
	// make sure caller only tries to set public bits
	// X_ASSERT(0 == (renderFlags.m_renderFlags & ~e_PublicParamsMask), "")();

	curRenderFlags_ = renderFlags;
}


X_INLINE XAuxGeomRenderFlags RenderAux::getRenderFlags(void)
{
	return curRenderFlags_;
}

// ---------------------------------------------------


X_INLINE uint32 RenderAux::createLineRenderFlags(bool indexed)
{
	if (indexed) {
		return (curRenderFlags_ | (PrimType::LineListInd << AuxGeomPrivateBitMasks::PrimTypeShift));
	}

	return (curRenderFlags_ | (PrimType::LineList << AuxGeomPrivateBitMasks::PrimTypeShift));
}


X_INLINE uint32 RenderAux::createTriangleRenderFlags(bool indexed)
{
	if (indexed) {
		return (curRenderFlags_ | (PrimType::TriListInd << AuxGeomPrivateBitMasks::PrimTypeShift));
	}

	return (curRenderFlags_ | (PrimType::TriList << AuxGeomPrivateBitMasks::PrimTypeShift));
}

X_INLINE uint32 RenderAux::createObjectRenderFlags(const DrawObjType::Enum objType)
{
	return (curRenderFlags_ | (PrimType::Obj << AuxGeomPrivateBitMasks::PrimTypeShift) | objType);
}



X_INLINE RenderAux::PrimType::Enum RenderAux::getPrimType(const XAuxGeomRenderFlags& renderFlags)
{
	uint32 primType = ((renderFlags & AuxGeomPrivateBitMasks::PrimTypeMask) >> AuxGeomPrivateBitMasks::PrimTypeShift);
	switch (primType)
	{
	case PrimType::PtList:
	case PrimType::LineList:
	case PrimType::LineListInd:
	case PrimType::TriList:
	case PrimType::TriListInd:
	case PrimType::Obj:
		return static_cast<PrimType::Enum>(primType);
	default:
	{
		X_ASSERT_UNREACHABLE();
	}
	}

	return PrimType::Obj;
}


X_INLINE bool RenderAux::isThickLine(const XAuxGeomRenderFlags& renderFlags)
{
	PrimType::Enum primType = getPrimType(renderFlags);
	X_ASSERT(PrimType::TriList == primType, "invalid primtype")(primType);

	if (PrimType::TriList == primType) {
		return	(0 != (renderFlags & AuxGeomPrivateRenderflags::TriListParam_ProcessThickLines));
	}

	return false;
}


X_INLINE RenderAux::DrawObjType::Enum RenderAux::getAuxObjType(const XAuxGeomRenderFlags& renderFlags)
{
	PrimType::Enum primType = getPrimType(renderFlags);
	X_ASSERT(PrimType::Obj == primType, "invalid primtype")(primType);

	uint32 objType = ((renderFlags & AuxGeomPrivateBitMasks::PrivateRenderflagsMask));

	switch (objType)
	{
	case DrawObjType::Sphere:
	case DrawObjType::Cone:
	case DrawObjType::Cylinder:
		return static_cast<DrawObjType::Enum>(objType);

	default:
	{
		X_ASSERT_UNREACHABLE();
	}
	}

	return (DrawObjType::Enum)objType;
}


X_INLINE uint8 RenderAux::getPointSize(const XAuxGeomRenderFlags& renderFlags)
{
	PrimType::Enum primType = getPrimType(renderFlags);
	X_ASSERT(PrimType::PtList == primType, "invalid primtype")(primType);

	if (PrimType::PtList == primType)
	{
		// this is not correct.
		X_ASSERT_NOT_IMPLEMENTED();
		// return (renderFlags & AuxGeomPrivateBitMasks::PrivateRenderflagsMask);
	}

	return 0;
}



X_NAMESPACE_END