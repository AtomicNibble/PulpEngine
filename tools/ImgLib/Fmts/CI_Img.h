#pragma once

#ifndef X_TEXTURE_CI_H_
#define X_TEXTURE_CI_H_

#include "Util\ReferenceCountedOwner.h"

X_NAMESPACE_BEGIN(texture)

namespace CI
{
    bool WriteCIImgAsync(core::Path<char>& path, core::ReferenceCountedOwner<XTextureFile>& image, core::MemoryArenaBase* arena);
    bool WriteCIImg(core::Path<char>& path, XTextureFile* image);

} // namespace CI

X_NAMESPACE_END

#endif // !X_TEXTURE_CI_H_