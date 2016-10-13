#pragma once

#include <ITexture.h>



X_NAMESPACE_BEGIN(texture)

class ImgLib : public IImgLib
{
public:
	ImgLib();
	~ImgLib() X_OVERRIDE;

	virtual const char* getOutExtension(void) const X_OVERRIDE;

	virtual bool Convert(IConverterHost& host, ConvertArgs& args, const core::Array<uint8_t>& fileData,
		const OutPath& destPath) X_OVERRIDE;


private:
};


X_NAMESPACE_END