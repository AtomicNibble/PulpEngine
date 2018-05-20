#pragma once

#include <ICore.h>
#include <Platform\TrayIcon.h>
#include <Platform\Module.h>

class EngineApp : public IAssertHandler
    , public core::TrayIcon
{
public:
    EngineApp();
    ~EngineApp() X_OVERRIDE;

    bool Init(HINSTANCE hInstance, core::MemoryArenaBase* arena, const wchar_t* sInCmdLine);
    bool ShutDown(void);
    bool PumpMessages(void);

private:
    virtual void OnAssert(const core::SourceInfo& sourceInfo) X_OVERRIDE;
    virtual void OnAssertVariable(const core::SourceInfo& sourceInfo) X_OVERRIDE;

private:
    virtual LRESULT OnTrayCmd(WPARAM wParam, LPARAM lParam) X_FINAL;

private:
    static void Error(const char* pErrorText);

private:
    bool run_;

    core::Module::Handle hSystemHandle_;
    ICore* pICore_;
};
