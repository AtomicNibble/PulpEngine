#pragma once

struct EngineGUID;
struct IEngineUnknown;

struct IEngineFactory
{
protected:
    virtual ~IEngineFactory() = default;

public:
    virtual const char* GetName(void) const X_ABSTRACT;
    virtual const EngineGUID& GetGUID(void) const X_ABSTRACT;
    virtual bool ClassSupports(const EngineGUID& guid) const X_ABSTRACT;
    virtual IEngineUnknown* CreateInstance(void) const X_ABSTRACT;
};
