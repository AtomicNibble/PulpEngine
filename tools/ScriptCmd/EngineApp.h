#pragma once

#include <ICore.h>
#include <Platform\Console.h>
#include <Platform\Module.h>

class EngineApp : public IAssertHandler
{
public:
    EngineApp();
    ~EngineApp() X_OVERRIDE;

    bool Init(HINSTANCE hInstance, core::MemoryArenaBase* arena, const wchar_t* pInCmdLine);
    bool ShutDown(void);

private:
    virtual void OnAssert(const core::SourceInfo& sourceInfo) X_OVERRIDE;
    virtual void OnAssertVariable(const core::SourceInfo& sourceInfo) X_OVERRIDE;

private:
    static void Error(core::string_view errorText);

private:
    core::Module::Handle hSystemHandle_;
    ICore* pICore_;
};
