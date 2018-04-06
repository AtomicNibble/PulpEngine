#include "EngineCommon.h"
#include "MatrixStack.h"

XMatrixStack::XMatrixStack(core::MemoryArenaBase* arena) :
    curDpeth_(0),
    maxDpeth_(0),
    pTop_(nullptr),
    pStack_(nullptr),
    arena_(arena)
{
}

XMatrixStack::~XMatrixStack()
{
    Clear();
}

void XMatrixStack::SetDepth(uint32_t maxDepth)
{
    Clear();

    pTop_ = pStack_ = X_NEW_ARRAY_ALIGNED(Matrix44f, maxDepth, arena_, "MatrixStack", X_ALIGN_OF(Matrix44f));
    maxDpeth_ = maxDepth;
}

void XMatrixStack::Clear(void)
{
    X_DELETE_ARRAY(pStack_, arena_);
    pTop_ = nullptr;
    pStack_ = nullptr;
    maxDpeth_ = 0;
    curDpeth_ = 0;
}

bool XMatrixStack::LoadIdentity(void)
{
    pTop_->setToIdentity();
    return true;
}

bool XMatrixStack::LoadMatrix(const Matrix44f* pMat)
{
    X_ASSERT_NOT_NULL(pMat);
    *pTop_ = *pMat;
    return true;
}

bool XMatrixStack::MultMatrix(const Matrix44f* pMat)
{
    X_ASSERT_NOT_NULL(pMat);
    *pTop_ *= *pMat;
    return true;
}

bool XMatrixStack::MultMatrixLocal(const Matrix44f* pMat)
{
    X_ASSERT_NOT_NULL(pMat);
    *pTop_ = *pMat * *pTop_;
    return true;
}

bool XMatrixStack::Pop(void)
{
    if (curDpeth_ == 0) {
        X_ASSERT(curDpeth_ > 0, "can't pop matrix off stack, none left")
        (curDpeth_);
        return false;
    }

    curDpeth_--;

    pTop_ = &pStack_[curDpeth_];
    return true;
}

bool XMatrixStack::Push(void)
{
    if ((curDpeth_ + 1) >= maxDpeth_) {
        X_ASSERT((curDpeth_ + 1) < maxDpeth_, "can't push matrix onto stack, at max depth")
        (curDpeth_, maxDpeth_);
        return false;
    }

    // copy stack down.
    memcpy(&pStack_[curDpeth_ + 1], &pStack_[curDpeth_], sizeof(Matrix44f));

    curDpeth_++;
    pTop_ = &pStack_[curDpeth_];
    return true;
}

bool XMatrixStack::RotateAxis(const Vec3f* pV, float32_t Angle)
{
    Matrix44f temp = Matrix44f::createRotation(*pV, Angle);
    temp.transpose();

    *pTop_ = *pTop_ * temp;
    return true;
}

bool XMatrixStack::RotateAxisLocal(const Vec3f* pV, float32_t Angle)
{
    Matrix44f temp = Matrix44f::createRotation(*pV, Angle);
    temp.transpose();

    *pTop_ = temp * (*pTop_);
    return true;
}

bool XMatrixStack::Scale(float yaw, float pitch, float roll)
{
    Matrix44f temp = Matrix44f::createScale(Vec3f(yaw, pitch, roll));

    *pTop_ = *pTop_ * temp;
    return true;
}

bool XMatrixStack::ScaleLocal(float yaw, float pitch, float roll)
{
    Matrix44f temp = Matrix44f::createScale(Vec3f(yaw, pitch, roll));

    *pTop_ = temp * (*pTop_);
    return true;
}

bool XMatrixStack::Translate(float x, float y, float z)
{
    Matrix44f temp = Matrix44f::createTranslation(Vec3f(x, y, z));
    temp.transpose();

    *pTop_ = *pTop_ * temp;
    return true;
}

bool XMatrixStack::TranslateLocal(float x, float y, float z)
{
    Matrix44f temp = Matrix44f::createTranslation(Vec3f(x, y, z));
    temp.transpose();

    *pTop_ = temp * (*pTop_);
    return true;
}
