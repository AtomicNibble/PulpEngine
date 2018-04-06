#include "stdafx.h"
#include "ShaderLib.h"

X_NAMESPACE_BEGIN(render)

namespace shader
{
    ShaderLib::ShaderLib()
    {
    }

    ShaderLib::~ShaderLib()
    {
    }

    const char* ShaderLib::getOutExtension(void) const
    {
        return "";
    }

    bool ShaderLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
    {
        X_UNUSED(host);
        X_UNUSED(assetId);
        X_UNUSED(args);
        X_UNUSED(destPath);
        return false;
    }

} // namespace shader

X_NAMESPACE_END
