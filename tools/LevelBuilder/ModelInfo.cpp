#include "stdafx.h"
#include "ModelInfo.h"

#include <IFileSys.h>
#include <IModel.h>

X_USING_NAMESPACE;

namespace ModelInfo
{

	bool GetNModelAABB(const core::string& name, AABB& boxOut)
	{
		core::Path<char> path;
		path /= "models/";
		path /= name;
		path.setExtension(model::MODEL_FILE_EXTENSION);

		core::fileModeFlags mode = core::fileMode::READ | core::fileMode::SHARE;
		core::XFileScoped file;

		boxOut.clear();

		if (file.openFile(path.c_str(), mode))
		{
#if 1
			X_ASSERT_NOT_IMPLEMENTED();
			return false;
#else
			model::ModelHeader hdr;
		
			if (file.readObj(hdr) != sizeof(hdr))
			{
				X_ERROR("ModelInfo", "Failed to read model header");
				return false;
			}

			if (!hdr.isValid())
			{
				X_ERROR("ModleInfo", "model hdr is not valid."
					" provided version: %i required: %i",
					hdr.version, model::MODEL_VERSION);
				return false;
			}


			boxOut = hdr.boundingBox;
			return true;
#endif
		}

		X_ERROR("ModelInfo", "Failed to open model file for reading.");
		return false;
	}


} // namespace ModelInfo