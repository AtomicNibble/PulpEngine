#pragma once


#include <Time\TimeVal.h>

#include <Math\XMatrix44.h>
#include <Math\XViewPort.h>

#include <IShader.h>

X_NAMESPACE_DECLARE(render,
	namespace shader
	{
		class XCBuffer;
	} // nmaespace shader
);

X_NAMESPACE_BEGIN(engine)

// this is for storing the automated values.
// so the idea is that we can just set the predefined values here and it works out if values have changed.
// if some values have changed that are part of the per frame const buffers it will schedule a update of the cbuf data to the render sys.
// this way we don't have to do redundancy checks in the render sys and we can store the redundancy check in a single place in 3degnine.
// instead of redundancy checks anywhere we update cbuffers that have like frame and batch update freq.
//
//	I'm not sure how i'm going to handle updating of cbuffers that are update per draw.
//	i don't want to have to update values in the manager before everydraw..
//	Or is that actually sane.. since it will just copy the value to destination.
//  I guess as long as it's limited or somthing.. humm..
//
//	I also want to maybe merge const buffers.
//	If a const buffer for two diffrent shaders is the same and is bound is same locations and everything.
//	they can both use the same gpu memory and we only have to update one.
//


class CBufferManager
{
public:
	CBufferManager();
	~CBufferManager();

	// updates any stale gpu buffers.
	void frameBegin(void);
	void batchBegin(void);

	void autoFillBuffer(render::shader::XCBuffer& cbuf);

	X_INLINE void setTime(core::TimeVal time);
	X_INLINE void setFrameTime(core::TimeVal time);
	X_INLINE void setViewPort(const XViewPort& viewport);
	X_INLINE void setViewMat(const Matrix44f& view);
	X_INLINE void setProjMat(const Matrix44f& proj);
	X_INLINE void setViewProjMat(const Matrix44f& view, const Matrix44f& proj);

private:
	X_INLINE void updateMatrixes(void);

private:
	render::shader::ParamTypeFlags dirtyParamFlags;

	core::TimeVal time_;
	core::TimeVal frameTime_;

	XViewPort viewPort_;

	Matrix44f view_;
	Matrix44f inView_;
	Matrix44f proj_;

	Matrix44f viewProj_;
};


X_INLINE void CBufferManager::setTime(core::TimeVal time)
{
	time_ = time;
}

X_INLINE void CBufferManager::setFrameTime(core::TimeVal frameTime)
{
	frameTime_ = frameTime;
}

X_INLINE void CBufferManager::setViewPort(const XViewPort& viewport)
{
	viewPort_ = viewport;
}

X_INLINE void CBufferManager::setViewMat(const Matrix44f& view)
{
	view_ = view;

	updateMatrixes();
}

X_INLINE void CBufferManager::setProjMat(const Matrix44f& proj)
{
	proj_ = proj;

	updateMatrixes();
}

X_INLINE void CBufferManager::setViewProjMat(const Matrix44f& view, const Matrix44f& proj)
{
	view_ = view;
	proj_ = proj;

	updateMatrixes();
}

X_INLINE void CBufferManager::updateMatrixes(void)
{
	inView_ = view_.inverted();
	viewProj_ = view_ * proj_;
}


X_NAMESPACE_END
