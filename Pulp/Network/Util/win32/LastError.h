#pragma once


X_NAMESPACE_BEGIN(net)

namespace lastError
{
	typedef char Description[512];

	int32_t Get(void);

	const char* ToString(int32_t error, Description& desc);
	const char* ToString(Description& desc);


} // lastError

X_NAMESPACE_END
