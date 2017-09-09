#pragma once


#include <maya\MColor.h>
#include <maya\MFloatVector.h>
#include <maya\MFloatPoint.h>
#include <maya\MMatrix.h>

namespace MayaUtil
{
	void SetStartOfBlock(bool start);
	void SetVerbose(bool verbose);
	bool IsVerbose(void);

	void MayaPrintError(const char *fmt, ...);
	void MayaPrintWarning(const char *fmt, ...);
	void MayaPrintMsg(const char *fmt, ...);
	void MayaPrintVerbose(const char *fmt, ...);

	void PrintMatrix(const char* pName, const Matrix33f& mat);
	void PrintMatrix(const char* pName, const MMatrix& mat);


	void SetProgressCtrl(const MString& str);
	MStatus ShowProgressDlg(int32_t progressMin, int32_t progressMax);
	MStatus SetProgressRange(int32_t progressMin, int32_t progressMax);
	MStatus IncProcess(void);
	MStatus HideProgressDlg(void);
	void SetProgressText(const MString& str, bool advProgress = true);


	MString RemoveNameSpace(const MString& str);


	X_INLINE ::std::ostream& operator<<(::std::ostream& os, const Vec3f& bar) {
		return os << "(" << bar.x << ", " << bar.y << ", " << bar.z << ")";
	}


	X_INLINE Vec3f ConvertToGameSpace(const Vec3f &pos) {
		Vec3f idpos;
		idpos.x = pos.x;
		idpos.y = pos.y;
		idpos.z = pos.z;
		return idpos;
	}

	X_INLINE Matrix33f ConvertToGameSpace(const Matrix33f& m) {
		Matrix33f mat;

		mat.m00 = m.m00;
		mat.m01 = m.m02;
		mat.m02 = m.m01;

		mat.m10 = m.m10;
		mat.m11 = m.m12;
		mat.m12 = m.m11;

		mat.m20 = m.m20;
		mat.m21 = m.m22;
		mat.m22 = m.m21;
		return mat;
	}

	X_INLINE Vec3f XVec(const MFloatVector& point) {
		return Vec3f(point[0], point[1], point[2]);
	}

	X_INLINE Vec3f XVec(const MFloatPoint& point) {
		return Vec3f(point[0], point[1], point[2]);
	}

	X_INLINE Color XVec(const MColor& col) {
		return Color(col[0], col[1], col[2], col[3]);
	}

	X_INLINE Vec3f XVec(const MMatrix& matrix) {
		return Vec3f((float)matrix[3][0], (float)matrix[3][1], (float)matrix[3][2]);
	}

	X_INLINE Matrix33f XMat(const MMatrix& matrix) {
		int		j, k;
		Matrix33f	mat;


		for (j = 0; j < 3; j++) {
			for (k = 0; k < 3; k++) {
				mat.at(j, k) = static_cast<float>(matrix(j,k));
			}
		}

		return mat;
	}


} // namespace MayaUtil