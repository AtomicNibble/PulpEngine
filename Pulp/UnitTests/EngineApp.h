#pragma once

#include <ICore.h>
#include <Platform\Module.h>

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

    bool Init(HINSTANCE hInstance, const wchar_t* pInCmdLine);
    bool ShutDown(void);

private:
    static void Error(core::string_view errorText);

private:
    AssetHandler assertCallback_;
    core::Module::Handle hSystemHandle_;
    ICore* pICore_;
};
