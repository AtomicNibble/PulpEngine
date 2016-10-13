#pragma once

#include <IMaterial.h>


X_NAMESPACE_BEGIN(engine)

class MaterialLib : public IMaterialLib
{
public:
	MaterialLib();
	~MaterialLib() X_OVERRIDE;

	virtual const char* getOutExtension(void) const X_OVERRIDE;

	virtual bool Convert(IConverterHost& host, ConvertArgs& args, const core::Array<uint8_t>& fileData,
		const OutPath& destPath) X_OVERRIDE;


private:
};


X_NAMESPACE_END