#pragma once

#ifndef X_MATRIX_STACK_H_
#define X_MATRIX_STACK_H_

#include "Math\XMatrix44.h"

// members from.
// http://msdn.microsoft.com/en-us/library/windows/desktop/bb174038(v=vs.85).aspx

class XMatrixStack
{
public:
	XMatrixStack(core::MemoryArenaBase* arena);
	~XMatrixStack();

	void SetDepth(uint32_t maxDepth);
	void Clear(void);

	X_INLINE Matrix44f* GetTop();

	bool LoadIdentity();
	bool LoadMatrix(const Matrix44f* pMat);

	bool MultMatrix(const Matrix44f* pMat);
	bool MultMatrixLocal(const Matrix44f* pMat);

	bool Pop();
	bool Push();

	bool RotateAxis(const Vec3f* pV, float32_t Angle);
	bool RotateAxisLocal(const Vec3f* pV, float32_t Angle);
//	bool RotateYawPitchRoll();
//	bool RotateYawPitchRollLocal();

	bool Scale(float yaw, float pitch, float roll);
	bool ScaleLocal(float yaw, float pitch, float roll);

	bool Translate(float x, float y, float z);
	bool TranslateLocal(float x, float y, float z);

	X_INLINE uint32_t getDepth(void) const {
		return curDpeth_;
	}

private:
	Matrix44f* pTop_;
	Matrix44f* pStack_;
	uint32_t curDpeth_;
	uint32_t maxDpeth_;

	core::MemoryArenaBase* arena_;
};


X_INLINE Matrix44f* XMatrixStack::GetTop()
{
	return pTop_;
}


#endif // !X_MATRIX_STACK_H_