#include <EngineCommon.h>
#include "Texture.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
    Texture::Texture(const char* pName, int16_t bindPoint, int16_t bindCount, texture::TextureType::Enum type) :
        name_(pName),
        bindPoint_(bindPoint),
        bindCount_(bindCount),
        type_(type)
    {
    }

    Texture::Texture(core::string& name, int16_t bindPoint, int16_t bindCount, texture::TextureType::Enum type) :
        name_(name),
        bindPoint_(bindPoint),
        bindCount_(bindCount),
        type_(type)
    {
    }

    bool Texture::SSave(core::XFile* pFile) const
    {
        pFile->writeString(name_);
        pFile->writeObj(bindPoint_);
        pFile->writeObj(bindCount_);
        pFile->writeObj(type_);
        return true;
    }

    bool Texture::SLoad(core::XFile* pFile)
    {
        pFile->readString(name_);
        pFile->readObj(bindPoint_);
        pFile->readObj(bindCount_);
        pFile->readObj(type_);
        return true;
    }

} // namespace shader

X_NAMESPACE_END
