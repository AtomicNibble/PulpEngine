#pragma once

#ifndef X_LASTERROR_H
#define X_LASTERROR_H


X_NAMESPACE_BEGIN(core)

/// \ingroup Util
/// \namespace lastError
/// \brief Wraps Windows-specific GetLastError functionality.
/// \details The functions in this namespace can be used to retrieve the last error set by certain Windows APIs, and
/// additionally offers functionality for converting those error codes into human-readable strings.
namespace lastError
{
	/// A character array used for holding a human-readable description of an error.
	typedef char Description[512];

	/// Returns the last error.
	unsigned int Get(void);

	/// \brief Converts any Windows-specific error code into a human-readable string, and returns a pointer to the description string.
	/// \remark The function does not allocate memory for the string. The returned pointer merely points to the \a desc
	/// character array so it can be used in other function calls.
	const char* ToString(DWORD error, Description& desc);

	/// \brief Converts the last error into a human-readable string, and returns a pointer to the description string.
	/// \remark The function does not allocate memory for the string. The returned pointer merely points to the \a desc
	/// character array so it can be used in other function calls.
	const char* ToString(Description& desc);
}

X_NAMESPACE_END


#endif
