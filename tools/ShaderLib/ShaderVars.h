#pragma once

X_NAMESPACE_DECLARE(core,
                    struct ICVar;)

X_NAMESPACE_BEGIN(render)

namespace shader
{
    class ShaderVars
    {
    public:
        ShaderVars();
        ~ShaderVars() = default;

        void RegisterVars(void);

        X_INLINE bool noSource(void) const;
        X_INLINE bool useCache(void) const;
        X_INLINE bool writeCompiledShaders(void) const;
        X_INLINE bool writeMergedSource(void) const;
        X_INLINE bool helpWithWorkOnShaderStall(void) const;
        X_INLINE bool compileDebug(void) const;

        X_INLINE void setUseCache(bool use);
        X_INLINE void setWriteCompiledShaders(bool write);

    private:
        int32_t noSource_;
        int32_t useCache_;
        int32_t writeCompiledShaders_;
        int32_t writeMergedSource_;
        int32_t helpWithWorkOnShaderStall_;
        int32_t compileDebug_;
    };

} // namespace shader

X_NAMESPACE_END

#include "ShaderVars.inl"