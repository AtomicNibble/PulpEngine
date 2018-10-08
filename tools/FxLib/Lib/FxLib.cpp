#include "stdafx.h"
#include "FxLib.h"

#include <IFileSys.h>

#include "Compiler\Compiler.h"

X_NAMESPACE_BEGIN(engine)

namespace fx
{
    FxLib::FxLib()
    {
    }

    FxLib::~FxLib()
    {
    }

    const char* FxLib::getOutExtension(void) const
    {
        return EFFECT_FILE_EXTENSION;
    }

    bool FxLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
    {
        X_UNUSED(host, assetId, args, destPath);

        EffectCompiler compiler(g_FxLibArena);

        if (!compiler.loadFromJson(args)) {
            X_ERROR("Fx", "Error parsing effect args");
            return false;
        }

        core::Array<AssetDep> dependencies(g_FxLibArena);
        if (!compiler.getDependencies(dependencies)) {
            return false;
        }

        if (!host.SetDependencies(assetId, dependencies)) {
            return false;
        }

        core::XFileScoped file;
        core::fileModeFlags mode = core::fileMode::RECREATE | core::fileMode::WRITE;

        if (!file.openFile(destPath.c_str(), mode)) {
            X_ERROR("Fx", "Failed to open output file");
            return false;
        }

        if (!compiler.writeToFile(file.GetFile())) {
            X_ERROR("Fx", "Failed to write effect file");
            return false;
        }

        return true;
    }

} // namespace fx

X_NAMESPACE_END