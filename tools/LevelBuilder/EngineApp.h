#pragma once

#include <ICore.h>
#include <Platform\Module.h>

X_NAMESPACE_DECLARE(physics,
                    struct IPhysLib;
                    struct IPhysicsCooking;)

class EngineApp : public IAssertHandler
{
public:
    EngineApp();
    ~EngineApp() X_OVERRIDE;

    bool Init(HINSTANCE hInstance, const wchar_t* pInCmdLine, core::MemoryArenaBase* arena);
    bool ShutDown(void);

    physics::IPhysicsCooking* GetPhysCooking(void);

private:
    virtual void OnAssert(const core::SourceInfo& sourceInfo) X_OVERRIDE;
    virtual void OnAssertVariable(const core::SourceInfo& sourceInfo) X_OVERRIDE;

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void Error(const char* pErrorText);

private:
    core::Module::Handle hSystemHandle_;
    ICore* pICore_;

    physics::IPhysLib* pPhysLib_;
    IConverterModule* pPhysConverterMod_;
};
