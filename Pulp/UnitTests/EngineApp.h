#pragma once


#include <ICore.h>
#include <Platform\Console.h>
#include <Platform\Module.h>

#define WIN_ENGINE_WINDOW_CLASSNAME "WinCatEngine"


class AssetHandler : public IAssertHandler
{
public:
	AssetHandler(void);
	virtual ~AssetHandler(void);

private:

	virtual void OnAssert(const core::SourceInfo& sourceInfo) X_OVERRIDE;
	virtual void OnAssertVariable(const core::SourceInfo& sourceInfo) X_OVERRIDE;
};



class EngineApp
{
public:
	EngineApp();
	~EngineApp();

	bool Init(HINSTANCE hInstance, const wchar_t* sInCmdLine, core::Console& Console);
	int	MainLoop();

private:
	static void Error(const char* pErrorText);

private:
	AssetHandler assertCallback_;
	core::Module::Handle hSystemHandle_;
	ICore* pICore_;
};

