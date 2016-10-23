#include "stdafx.h"
#include "MaterialLib.h"

#include "Compiler\Compiler.h"

X_NAMESPACE_BEGIN(engine)


MaterialLib::MaterialLib()
{


}

MaterialLib ::~MaterialLib()
{
}



const char* MaterialLib::getOutExtension(void) const
{
	return engine::MTL_B_FILE_EXTENSION;
}

bool MaterialLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
{
	X_UNUSED(host);
	X_ASSERT_NOT_NULL(gEnv);

	// we basically just take the json and compile it into a binary format.

	MaterialCompiler compiler;

	if (!compiler.loadFromJson(args)) {
		X_ERROR("Mat", "Error parsing material args");
		return false;
	}



	return false;
}


X_NAMESPACE_END