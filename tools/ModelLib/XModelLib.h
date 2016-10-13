#pragma once


#include <IModel.h>

X_NAMESPACE_BEGIN(model)

class XModelLib : public IModelLib
{
public:
	XModelLib();
	~XModelLib() X_OVERRIDE;

	virtual const char* getOutExtension(void) const X_OVERRIDE;

	bool Convert(IConverterHost& host, ConvertArgs& args, const core::Array<uint8_t>& fileData,
		const OutPath& destPath) X_OVERRIDE;


private:
};


X_NAMESPACE_END