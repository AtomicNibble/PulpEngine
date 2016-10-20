#pragma once

#include <IMaterial.h>


X_NAMESPACE_BEGIN(engine)

class MaterialLib : public IMaterialLib
{
public:
	MaterialLib();
	~MaterialLib() X_OVERRIDE;

	virtual const char* getOutExtension(void) const X_OVERRIDE;

	virtual bool Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath) X_OVERRIDE;


private:
};


X_NAMESPACE_END