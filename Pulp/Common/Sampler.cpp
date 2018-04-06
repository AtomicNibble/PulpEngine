#include <EngineCommon.h>
#include "Sampler.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
    Sampler::Sampler(const char* pName, int16_t bindPoint, int16_t bindCount) :
        name_(pName),
        bindPoint_(bindPoint),
        bindCount_(bindCount)
    {
    }

    Sampler::Sampler(core::string& name, int16_t bindPoint, int16_t bindCount) :
        name_(name),
        bindPoint_(bindPoint),
        bindCount_(bindCount)
    {
    }

    bool Sampler::SSave(core::XFile* pFile) const
    {
        pFile->writeString(name_);
        pFile->writeObj(bindPoint_);
        pFile->writeObj(bindCount_);
        return true;
    }

    bool Sampler::SLoad(core::XFile* pFile)
    {
        pFile->readString(name_);
        pFile->readObj(bindPoint_);
        pFile->readObj(bindCount_);
        return true;
    }

} // namespace shader

X_NAMESPACE_END
