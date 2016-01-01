#pragma once


#include <IAnimation.h>

X_NAMESPACE_BEGIN(anim)

class XAnimLib : public IAnimLib
{
public:
	XAnimLib();
	~XAnimLib() X_OVERRIDE;

	virtual bool ConvertAnim(const char* pAnimInter,
		const char* pModel, const char* pDest) X_OVERRIDE;


private:
};


X_NAMESPACE_END