#include "stdafx.h"
#include "MaterialLib.h"

#include "IFileSys.h"

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

	core::XFileScoped file;
	core::fileModeFlags mode = core::fileMode::RECREATE | core::fileMode::WRITE;

	if (!file.openFile(destPath.c_str(), mode)) {

		return false;
	}
	
	if(!compiler.writeToFile(file.GetFile())) {

		return false;
	}

	return true;
}


X_NAMESPACE_END