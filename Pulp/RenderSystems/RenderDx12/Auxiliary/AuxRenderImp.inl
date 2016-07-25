

X_NAMESPACE_BEGIN(render)


X_INLINE const Matrix44f& RenderAuxImp::getCurrentView(void) const
{
	return matrices_.matView;
}

X_INLINE const Matrix44f& RenderAuxImp::getCurrentViewInv(void) const
{
	return matrices_.matViewInv;
}

X_INLINE const Matrix44f& RenderAuxImp::getCurrentProj(void) const
{
	return matrices_.matProj;
}

X_INLINE const Matrix44f& RenderAuxImp::getCurrentTrans3D(void) const
{
	return matrices_.matTrans2D;
}

X_INLINE const Matrix44f& RenderAuxImp::getCurrentTrans2D(void) const
{
	return matrices_.matTrans2D;
}


X_INLINE const RenderAux::AuxVertexArr& RenderAuxImp::getAuxVertexBuffer(void) const
{
	X_ASSERT_NOT_NULL(pCurCBRawData_);
	return pCurCBRawData_->auxVertexArr;
}

X_INLINE const RenderAux::AuxIndexArr& RenderAuxImp::getAuxIndexBuffer(void) const
{
	X_ASSERT_NOT_NULL(pCurCBRawData_);
	return pCurCBRawData_->auxIndexArr;
}

X_INLINE const RenderAux::AuxDrawObjParamArr& RenderAuxImp::getAuxDrawObjParamBuffer(void) const
{
	X_ASSERT_NOT_NULL(pCurCBRawData_);
	return pCurCBRawData_->auxDrawObjParamArr;
}

X_INLINE const Matrix44f& RenderAuxImp::getAuxOrthoMatrix(int32_t idx) const
{
	X_ASSERT_NOT_IMPLEMENTED();
	return Matrix44f::identity();
}

X_INLINE bool RenderAuxImp::isOrthoMode(void) const
{
	return false;
}




X_NAMESPACE_END