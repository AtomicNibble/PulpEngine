#pragma once


#include <IDirectoryWatcher.h>

#include <Containers\HashMap.h>
#include <Containers\Array.h>

#include <Assets\AssertContainer.h>

#include "ShaderBin.h"
#include "ShaderVars.h"
#include "ILTree.h"

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
		typedef core::HashMap<core::string, SourceFile*> ShaderSourceMap;
		typedef core::HashMap<core::string, XHWShader*> HWShaderMap;
		typedef core::XResourceContainer ShaderCon;

	public:
		XShaderManager(core::MemoryArenaBase* arena);
		~XShaderManager();

		bool init(void);
		bool shutdown(void);

		// loads shader if not already loaded.
		XShader* forName(const char* pName);

		// returns merged source.
		bool sourceToString(const char* pName, core::string& strOut);

		ILTreeNode& getILTree(void);
		ShaderVars& getShaderVars(void);

	private:
		bool loadCoreShaders(void);
		bool freeCoreShaders(void);


		// returns a loaded shader, null if not fnd.
		XShader* getLoadedShader(const char* pName);

		ShaderSourceFile* loadShaderFile(const char* pName, bool reload = false);
		SourceFile* loadRawSourceFile(const char* pName, bool reload = false);

		void parseIncludesAndPrePro_r(SourceFile* file, core::Array<SourceFile*>& includedFiles,
			bool reload = false);

		XShader* createShader(const char* pName);
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

		void createInputLayoutTree(void);

	private:
		// IXHotReload
		void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
		// ~IXHotReload

	private:
		void writeSourceToFile(core::XFile* pFile, const SourceFile* pSource);

		void Cmd_ListShaders(core::IConsoleCmdArgs* pArgs);
		void Cmd_ListShaderSources(core::IConsoleCmdArgs* pArgs);

	private:
		core::MemoryArenaBase* arena_;
		core::Crc32* pCrc32_;
		ShaderSourceMap sourcebin_;
		ShaderBin shaderBin_;
		ShaderCon shaders_;
		HWShaderMap hwShaders_;

		ILTreeNode ilRoot_;	
		ShaderVars vars_;

	private:
		XShader* pDefaultShader_;
		XShader* pFixedFunction_;
		XShader* pFont_;
		XShader* pGui_;
	};



} // namespace shader

X_NAMESPACE_END