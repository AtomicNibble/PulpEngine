#include "stdafx.h"
#include "RawModelTypes.h"
#include "RawModel.h"



X_NAMESPACE_BEGIN(model)



bool RawModel::LoadRawModel(core::Path<char>& path)
{
	core::Path<wchar_t> widePath(path);

	return LoadRawModel(widePath);
}


bool RawModel::LoadRawModel(core::Path<wchar_t>& path)
{

	return false;
}

X_NAMESPACE_END