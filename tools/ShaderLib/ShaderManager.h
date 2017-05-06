#pragma once


#include <IDirectoryWatcher.h>

#include <Containers\HashMap.h>
#include <Containers\Array.h>

#include <Assets\AssertContainer.h>

#include "ShaderVars.h"


X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(render)

namespace shader
{

	class SourceFile;
	class XShader;
	class XHWShader;

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
		> PoolArena;


	public:
		XShaderManager(core::MemoryArenaBase* arena);
		~XShaderManager();

		bool init(void);
		bool shutDown(void);
		
		IShaderSource* sourceforName(const char* pSourceName);
		IHWShader* createHWShader(shader::ShaderType::Enum type, const core::string& entry, 
			shader::IShaderSource* pSourceFile, shader::PermatationFlags permFlags);
		void releaseHWShader(IHWShader* pHWSHader);

		shader::IShaderPermatation* createPermatation(const shader::ShaderStagesArr& stages);
		void releaseShaderPermatation(shader::IShaderPermatation* pPerm);

		ShaderVars& getShaderVars(void);
		ShaderBin& getBin(void);

	private:
		SourceFile* loadRawSourceFile(const char* pName, bool reload = false);

		XHWShader* hwForName(ShaderType::Enum type, const core::string& entry,
			SourceFile* pSourceFile, const TechFlags techFlags, ILFlags ILFlags);
		void releaseHWShader(XHWShader* pHWSHader);

	private:
		static void getShaderCompileSrc(XHWShader* pShader, core::Path<char>& srcOut);


		bool freeSourcebin(void);
		bool freeHwShaders(void);
		void listHWShaders(const char* pSarchPatten = nullptr);
		void listShaderSources(const char* pSarchPatten = nullptr);

	private:
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
		core::HeapArea      permHeap_;
		core::PoolAllocator permAllocator_;
		PoolArena			permArena_;

		ShaderVars vars_;
	};



} // namespace shader

X_NAMESPACE_END