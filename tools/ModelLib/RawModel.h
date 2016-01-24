#pragma once


#include <String\Path.h>

X_NAMESPACE_BEGIN(model)

class RawModel
{
public:
	RawModel() = default;
	~RawModel() = default;

	bool LoadRawModel(core::Path<char>& path);
	bool LoadRawModel(core::Path<wchar_t>& path);

private:

};


X_NAMESPACE_END