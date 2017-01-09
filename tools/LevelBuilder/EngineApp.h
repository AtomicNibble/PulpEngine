#pragma once


#include <ICore.h>
#include <Platform\Console.h>
#include <Platform\Module.h>


X_NAMESPACE_DECLARE(physics,
	struct IPhysLib;
	struct IPhysicsCooking;
)


class EngineApp : public IAssertHandler
{
public:
	EngineApp();
	~EngineApp() X_OVERRIDE;

	bool Init(const wchar_t* sInCmdLine, core::Console& Console);
	bool ShutDown(void);

	physics::IPhysicsCooking* GetPhysCooking(void);

private:
	virtual void OnAssert(const core::SourceInfo& sourceInfo) X_OVERRIDE;
	virtual void OnAssertVariable(const core::SourceInfo& sourceInfo) X_OVERRIDE;

private:

	static LRESULT CALLBACK	WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	core::Module::Handle hSystemHandle_;
	ICore* pICore_;

	physics::IPhysLib* pPhysLib_;
	IConverterModule* pPhysConverterMod_;
};

