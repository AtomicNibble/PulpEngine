#pragma once


#include <IAnimation.h>

X_NAMESPACE_BEGIN(anim)

class XAnimLib : public IAnimLib
{
public:
	XAnimLib();
	~XAnimLib() X_OVERRIDE;

	virtual bool Convert(ConvertArgs& args, const core::Array<uint8_t>& fileData, 
		const OutPath& destPath) X_OVERRIDE;


private:
};


X_NAMESPACE_END