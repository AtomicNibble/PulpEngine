#pragma once

#include <ICore.h>
#include <Platform\Console.h>
#include <Platform\Module.h>

#include <Memory\SimpleMemoryArena.h>

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
    typedef core::SimpleMemoryArena<
        core::MallocFreeAllocator>
        Arena;

public:
    EngineApp();
    ~EngineApp();

    bool Init(HINSTANCE hInstance, const wchar_t* sInCmdLine, core::Console& Console);
    bool ShutDown(void);

private:
    static void Error(const char* pErrorText);

private:
    core::MallocFreeAllocator allocator_;
    Arena arena_;

    AssetHandler assertCallback_;
    core::Module::Handle hSystemHandle_;
    ICore* pICore_;
};
