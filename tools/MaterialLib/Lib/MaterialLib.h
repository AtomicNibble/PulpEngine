#pragma once

#include <IMaterial.h>

#include <Util\UniquePointer.h>

X_NAMESPACE_BEGIN(engine)

namespace techset
{
    class TechSetDefs;

} // namespace techset

class MaterialLib : public IMaterialLib
{
public:
    MaterialLib();
    ~MaterialLib() X_OVERRIDE;

    virtual const char* getOutExtension(void) const X_OVERRIDE;

    virtual bool Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath) X_OVERRIDE;

private:
    core::UniquePointer<techset::TechSetDefs> pTechDefs_;
};

X_NAMESPACE_END