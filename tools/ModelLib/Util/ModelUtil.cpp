#include "stdafx.h"
#include "ModelUtil.h"

#include <IFileSys.h>

#include <IModel.h>

X_NAMESPACE_BEGIN(model)

namespace Util
{
    bool GetModelAABB(const core::Path<char>& path, AABB& boxOut)
    {
        core::FileFlags mode = core::FileFlag::READ | core::FileFlag::SHARE;
        core::XFileScoped file;

        boxOut.clear();

        if (file.openFile(path, mode)) {
            ModelHeader hdr;

            if (file.readObj(hdr) != sizeof(hdr)) {
                X_ERROR("Model", "Failed to read model header");
                return false;
            }

            if (!hdr.isValid()) {
                X_ERROR("Model", "model hdr is not valid. provided version: %i required: %i",
                    hdr.version, model::MODEL_VERSION);
                return false;
            }

            boxOut = hdr.boundingBox;
            return true;
        }

        X_ERROR("Model", "Failed to open model file for reading.");
        return false;
    }

} // namespace Util

X_NAMESPACE_END