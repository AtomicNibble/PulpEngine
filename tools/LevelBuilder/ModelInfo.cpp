#include "stdafx.h"
#include "ModelInfo.h"

#include <IFileSys.h>
#include <IModel.h>

X_USING_NAMESPACE;

namespace ModelInfo
{

	bool GetNModelAABB(const core::string& name, AABB& boxOut)
	{
		core::Path path;
		path /= name;

		core::fileModeFlags mode = core::fileMode::READ;
		core::XFileScoped file;

		boxOut.clear();

		if (file.openFile(path.c_str(), mode))
		{
			model::ModelHeader hdr;
		
			if (file.readObj(hdr) != sizeof(hdr))
			{
				X_ERROR("ModelInfo", "Failed to read model header");
				return false;
			}

			if (!hdr.isValid())
			{
				X_ERROR("ModleInfo", "model info is not valid");
				return false;
			}


			boxOut = hdr.boundingBox;
			return true;
		}

		X_ERROR("ModelInfo", "Failed to open model file for reading.");
		return false;
	}


} // namespace ModelInfo