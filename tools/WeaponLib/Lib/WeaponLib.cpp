#include "stdafx.h"
#include "WeaponLib.h"
#include "Compiler\Compiler.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{
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

        core::Array<AssetDep> dependencies(g_WeaponLibArena);

        WeaponCompiler compiler;

        if (!compiler.loadFromJson(args)) {
            X_ERROR("Weapon", "Error parsing weapon args");
            return false;
        }

        if (!compiler.getDependencies(dependencies)) {
            X_ERROR("Weapon", "Failed to get dependencies");
            return false;
        }

        core::XFileScoped file;
        core::FileFlags mode = core::FileFlag::RECREATE | core::FileFlag::WRITE;

        if (!file.openFile(destPath.c_str(), mode)) {
            X_ERROR("Weapon", "Failed to open output file");
            return false;
        }

        if (!compiler.writeToFile(file.GetFile())) {
            X_ERROR("Weapon", "Failed to write weapon file");
            return false;
        }

        if (dependencies.isNotEmpty()) {
            if (!host.SetDependencies(assetId, core::make_span(dependencies))) {
                return false;
            }
        }

        return true;
    }

} // namespace weapon

X_NAMESPACE_END