#pragma once


#include <IDirectoryWatcher.h>

#include <Containers\HashMap.h>
#include <Containers\Array.h>

#include <Assets\AssertContainer.h>

#include "Shader.h"
#include "ShaderVars.h"


X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(render)

namespace shader
{

	class SourceFile;
	class ShaderSourceFile;
	class XShader;
	class XHWShader;

	class XShaderManager : public core::IXHotReload
	{
		// Shaders
		typedef core::AssetContainer<XShader, MAX_SHADERS, core::MultiThreadPolicy<core::Spinlock>> ShaderContainer;
		typedef ShaderContainer::Resource ShaderResource;

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
		> PoolArena;


	public:
		XShaderManager(core::MemoryArenaBase* arena);
		~XShaderManager();

		bool init(void);
		bool shutDown(void);

		// loads shader if not already loaded.
		XShader* forName(const char* pName);
		void releaseShader(XShader* pShader);

		// returns merged source.
		bool sourceToString(const char* pName, core::string& strOut);

		ShaderVars& getShaderVars(void);
		ShaderBin& getBin(void);


	private:
		bool loadCoreShaders(void);
		bool freeCoreShaders(void);
		bool freeDanglingShaders(void);


		// returns a loaded shader, null if not fnd.
		ShaderResource* getLoadedShader(const char* pName);

		ShaderSourceFile* loadShaderFile(const char* pName, bool reload = false);
		SourceFile* loadRawSourceFile(const char* pName, bool reload = false);


		ShaderResource* createShader(const char* pName);
		XShader* loadShader(const char* pName);
		XShader* reloadShader(const char* pName);

		XHWShader* hwForName(ShaderType::Enum type, const char* pShaderName, const core::string& entry,
			SourceFile* pSourceFile, const TechFlags techFlags,
			ILFlags ILFlags);

	private:
		bool freeSourcebin(void);
		bool freeSourceHwShaders(void);
		void listShaders(const char* pSarchPatten = nullptr);
		void listShaderSources(const char* pSarchPatten = nullptr);

	private:
		// IXHotReload
		void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
		// ~IXHotReload

	private:

		void Cmd_ListShaders(core::IConsoleCmdArgs* pArgs);
		void Cmd_ListShaderSources(core::IConsoleCmdArgs* pArgs);

	private:
		core::MemoryArenaBase* arena_;
		core::Crc32* pCrc32_;
		ShaderBin shaderBin_;
		SourceBin sourceBin_;

		// allocator for source objects.
		core::HeapArea      sourcePoolHeap_;
		core::PoolAllocator sourcePoolAllocator_;
		PoolArena			sourcePoolArena_;

		// ref counted resources
		HWShaderContainer hwShaders_;
		ShaderContainer shaders_;

		ShaderVars vars_;

	private:
		XShader* pDefaultShader_;
		XShader* pFixedFunction_;
		XShader* pFont_;
		XShader* pGui_;
	};



} // namespace shader

X_NAMESPACE_END