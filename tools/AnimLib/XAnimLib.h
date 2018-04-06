#pragma once

#include <IAnimation.h>

X_NAMESPACE_BEGIN(anim)

class XAnimLib : public IAnimLib
{
public:
    XAnimLib();
    ~XAnimLib() X_OVERRIDE;

    virtual const char* getOutExtension(void) const X_OVERRIDE;

    virtual bool Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath) X_OVERRIDE;

private:
};

X_NAMESPACE_END