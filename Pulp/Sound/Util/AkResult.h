#pragma once


X_NAMESPACE_BEGIN(sound)



namespace AkResult
{
	typedef core::StackString<128, char> Description;

	const char* ToString(AKRESULT res, Description& desc);
}


X_NAMESPACE_END