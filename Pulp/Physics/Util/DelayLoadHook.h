#pragma once

#include <common\windows\PxWindowsDelayLoadHook.h>

X_NAMESPACE_BEGIN(physics)

class DelayLoadHook : public physx::PxDelayLoadHook
{
public:
    enum class Config
    {
        Normal, // load same config this dll was built as.
        Debug,
        Checked,
        Profile,
        Release
    };

    X_DECLARE_ENUM(NameBuffer)
    (
        Physx,
        PhysxCommon,
        PhysxCooking,
        PhysxCharacterKinematic);

public:
    DelayLoadHook();

    void forceConfig(Config config);
    Config getConfig(void) const;

    virtual const char* getPhysXCommonDEBUGDllName(void) const X_FINAL;
    virtual const char* getPhysXCommonCHECKEDDllName(void) const X_FINAL;
    virtual const char* getPhysXCommonPROFILEDllName(void) const X_FINAL;
    virtual const char* getPhysXCommonDllName(void) const X_FINAL;

    const char* getPhysxName(const char* pRequestedName);

private:
    const char* createNameForConfig(const char* pPrefix, core::Path<char>& buffer);
    const char* getCommonDllName(Config config) const;

private:
    Config config_;

    // these are just so we are thread safe for each delay load dll.
    core::Path<char> dllNames_[NameBuffer::ENUM_COUNT];
};

extern DelayLoadHook gDelayLoadHook;

X_NAMESPACE_END