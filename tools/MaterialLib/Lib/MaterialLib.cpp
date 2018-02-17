#include "stdafx.h"
#include "MaterialLib.h"

#include <IFileSys.h>

#include "Compiler\Compiler.h"

#include "TechDefs\TechDefs.h"

X_NAMESPACE_BEGIN(engine)


MaterialLib::MaterialLib() :
	pTechDefs_(g_MatLibArena)
{


}

MaterialLib::~MaterialLib()
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

	// lazy make the techDef loader.
	if (!pTechDefs_) {
		pTechDefs_ = core::makeUnique<techset::TechSetDefs>(g_MatLibArena, g_MatLibArena);
	}

	// we basically just take the json and compile it into a binary format.

	MaterialCompiler compiler(*pTechDefs_.get());

	if (!compiler.loadFromJson(args)) {
		X_ERROR("Mat", "Error parsing material args");
		return false;
	}

	core::XFileScoped file;
	core::fileModeFlags mode = core::fileMode::RECREATE | core::fileMode::WRITE;

	if (!file.openFile(destPath.c_str(), mode)) {
		X_ERROR("Mat", "Failed to open output file");
		return false;
	}
	
	if(!compiler.writeToFile(file.GetFile())) {
		X_ERROR("Mat", "Failed to write material file");
		return false;
	}

	return true;
}


X_NAMESPACE_END