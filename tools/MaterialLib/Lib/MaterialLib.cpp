#include "stdafx.h"
#include "MaterialLib.h"


X_NAMESPACE_BEGIN(engine)


MaterialLib::MaterialLib()
{


}

MaterialLib ::~MaterialLib()
{
}



const char* MaterialLib::getOutExtension(void) const
{
	return engine::MTL_B_FILE_EXTENSION;
}

bool MaterialLib::Convert(IConverterHost& host, ConvertArgs& args, const core::Array<uint8_t>& fileData,
	const OutPath& destPath)
{
	X_UNUSED(host);
	X_ASSERT_NOT_NULL(gEnv);
	// don't think i'll be using the job ystem for material conversion :D
//	X_ASSERT_NOT_NULL(gEnv->pJobSys);

	core::json::Document d;
	d.Parse(args.c_str());


	if (fileData.isEmpty()) {
		X_ERROR("MaterialLib", "File data is empty");
		return false;
	}



	return false;
}


X_NAMESPACE_END