#pragma once




X_NAMESPACE_BEGIN(render)

namespace Error
{
	typedef char Description[512];

	const char* ToString(HRESULT error, Description& desc);

}

X_NAMESPACE_END

