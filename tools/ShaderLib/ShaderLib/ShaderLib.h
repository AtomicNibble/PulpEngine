#pragma once

#include <IShader.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
    class ShaderLib : public IShaderLib
    {
    public:
        ShaderLib();
        ~ShaderLib() X_OVERRIDE;

        virtual const char* getOutExtension(void) const X_OVERRIDE;

        virtual bool Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath) X_OVERRIDE;

    private:
    };

} // namespace shader

X_NAMESPACE_END