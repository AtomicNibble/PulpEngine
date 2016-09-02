#include "stdafx.h"
#include "ImgLib.h"



X_NAMESPACE_BEGIN(texture)

ImgLib::ImgLib()
{

}

ImgLib::~ImgLib()
{

}

bool ImgLib::Convert(IConverterHost& host, ConvertArgs& args, const core::Array<uint8_t>& fileData,
	const OutPath& destPath)
{
	X_UNUSED(host);
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pJobSys);

	core::json::Document d;
	d.Parse(args.c_str());


	if (fileData.isEmpty()) {
		X_ERROR("ImgLib", "File data is empty");
		return false;
	}



	return false;
}

X_NAMESPACE_END