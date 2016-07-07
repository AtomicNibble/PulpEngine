#include "EngineCommon.h"
#include "XCamera.h"


void XCamera::UpdateFrustum(void)
{
	XFrustum::UpdateFrustum();

	// proj
	MatrixPerspectiveFovRH(&projectionMatrix_, fov_, projectionRatio_, near_, far_);

	// view
	const Matrix34f& m = mat_;
	Vec3f eye = getPosition();
	Vec3f at = eye + Vec3f(m.m01, m.m11, m.m21);
	Vec3f up = Vec3f(m.m02, m.m12, m.m22);

	MatrixLookAtRH(&viewMatrix_, eye, at, up);
}

