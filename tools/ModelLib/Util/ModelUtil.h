#pragma once

X_NAMESPACE_BEGIN(model)

namespace Util
{
    MODELLIB_EXPORT bool GetModelAABB(const core::Path<char>& path, AABB& boxOut);

} // namespace Util

X_NAMESPACE_END