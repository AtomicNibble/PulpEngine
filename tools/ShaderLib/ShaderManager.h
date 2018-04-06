#pragma once

#include <IShader.h>
#include <IDirectoryWatcher.h>

#include <Containers\HashMap.h>
#include <Containers\Array.h>

#include <Assets\AssertContainer.h>

#include "ShaderVars.h"
#include "ShaderBin.h"
#include "SourceBin.h"

X_NAMESPACE_DECLARE(core,
                    namespace V2 {
                        struct Job;
                        class JobSystem;
                    }

                    struct IoRequestBase;
                    struct XFileAsync;
                    struct IConsoleCmdArgs;)

X_NAMESPACE_BEGIN(render)

namespace shader
{
    class SourceFile;
    class XShader;
    class XHWShader;
    class ShaderPermatation;

    class XShaderManager : public core::IXHotReload
    {
        // HWShaders
        typedef core::AssetContainer<XHWShader, MAX_HW_SHADERS, core::MultiThreadPolicy<core::Spinlock>> HWShaderContainer;
        typedef HWShaderContainer::Resource HWShaderResource;

        // Shader Source
        typedef core::MemoryArena<
            core::PoolAllocator,
            core::MultiThreadPolicy<core::Spinlock>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
            core::SimpleBoundsChecking,
            core::SimpleMemoryTracking,
            core::SimpleMemoryTagging
#else
            core::NoBoundsChecking,
            core::NoMemoryTracking,
            core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
            >
            PoolArena;

        struct CompileJobInfo
        {
            CompileJobInfo()
            {
                core::zero_this(this);
            }
            CompileJobInfo(XHWShader* pHWShader, CompileFlags flags) :
                pHWShader(pHWShader),
                flags(flags),
                result(false)
            {
            }

            XHWShader* pHWShader;
            CompileFlags flags;
            bool result;
        };

    public:
        SHADERLIB_EXPORT XShaderManager(core::MemoryArenaBase* arena);
        SHADERLIB_EXPORT ~XShaderManager();

        SHADERLIB_EXPORT void registerVars(void);
        SHADERLIB_EXPORT void registerCmds(void);

        SHADERLIB_EXPORT bool init(void);
        SHADERLIB_EXPORT bool shutDown(void);

        SHADERLIB_EXPORT IShaderSource* sourceforName(const core::string& name);
        SHADERLIB_EXPORT XHWShader* createHWShader(shader::ShaderType::Enum type, const core::string& entry,
            const core::string& customDefines, shader::IShaderSource* pSourceFile,
            shader::PermatationFlags permFlags, render::shader::VertexFormat::Enum vertFmt);
        SHADERLIB_EXPORT XHWShader* createHWShader(shader::ShaderType::Enum type, const core::string& entry,
            const core::string& customDefines, shader::IShaderSource* pSourceFile,
            shader::PermatationFlags permFlags, ILFlags ILFlags);
        SHADERLIB_EXPORT void releaseHWShader(XHWShader* pHWSHader);

        SHADERLIB_EXPORT bool compileShader(XHWShader* pHWShader, CompileFlags flags);
        SHADERLIB_EXPORT shader::IShaderPermatation* createPermatation(const shader::ShaderStagesArr& stages);
        SHADERLIB_EXPORT bool compilePermatation(shader::IShaderPermatation* pPerm); // used to recompile a perm, hot reloading only
        SHADERLIB_EXPORT void releaseShaderPermatation(shader::IShaderPermatation* pPerm);

        X_INLINE ShaderVars& getShaderVars(void);
        X_INLINE ShaderBin& getBin(void);

    private:
        bool compilePermatation_Int(ShaderPermatation* pPerm);

        void compileShader_job(CompileJobInfo* pJobInfo, uint32_t num);

        XHWShader* hwForName(ShaderType::Enum type, const core::string& entry, const core::string& customDefines,
            SourceFile* pSourceFile, const shader::PermatationFlags permFlags, ILFlags ILFlags);

    private:
        static void getShaderCompileSrc(XHWShader* pShader, core::Path<char>& srcOut);

        void freeSourcebin(void);
        void freeHwShaders(void);
        void listHWShaders(const char* pSarchPatten = nullptr);
        void listShaderSources(const char* pSarchPatten = nullptr);

    private:
        void IoCallback(core::IFileSys&, const core::IoRequestBase*, core::XFileAsync*, uint32_t);

        // IXHotReload
        void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
        // ~IXHotReload

    private:
        void Cmd_ListHWShaders(core::IConsoleCmdArgs* pArgs);
        void Cmd_ListShaderSources(core::IConsoleCmdArgs* pArgs);

    private:
        core::MemoryArenaBase* arena_;
        ShaderBin shaderBin_;
        SourceBin sourceBin_;

        // ref counted resources
        HWShaderContainer hwShaders_;

        // allocator for source objects.
        core::HeapArea permHeap_;
        core::PoolAllocator permAllocator_;
        PoolArena permArena_;

        ShaderVars vars_;
    };

} // namespace shader

X_NAMESPACE_END

#include "ShaderManager.inl"