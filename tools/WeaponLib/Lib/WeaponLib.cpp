#include "stdafx.h"
#include "WeaponLib.h"
#include "Compiler\Compiler.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(game)


WeaponLib::WeaponLib()
{
}

WeaponLib::~WeaponLib()
{

}

const char* WeaponLib::getOutExtension(void) const
{
	return WEAPON_FILE_EXTENSION;
}

bool WeaponLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
{
	X_UNUSED(host, assetId, args, destPath);


	WeaponCompiler compiler;

	if (!compiler.loadFromJson(args)) {
		X_ERROR("Weapon", "Error parsing weapon args");
		return false;
	}

	core::XFileScoped file;
	core::fileModeFlags mode = core::fileMode::RECREATE | core::fileMode::WRITE;

	if (!file.openFile(destPath.c_str(), mode)) {
		X_ERROR("Weapon", "Failed to open output file");
		return false;
	}

	if (!compiler.writeToFile(file.GetFile())) {
		X_ERROR("Weapon", "Failed to write weapon file");
		return false;
	}

	return true;
}


X_NAMESPACE_END