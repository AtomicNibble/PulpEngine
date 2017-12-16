#pragma once

#ifndef X_ASSERT_H_
#define X_ASSERT_H_

#include <Util/SourceInfo.h>


X_NAMESPACE_BEGIN(core)


class Assert
{
public:
	/// Constructs an instance, dispatching the source info and the formatted messages using logDispatch and assertionDispatch.
	Assert(const SourceInfo& sourceInfo, const char* format, ...);

	/// Dispatches the name and value of a bool variable.
	Assert& Variable(const char* const name, bool var);

	/// Dispatches the name and value of a char variable.
	Assert& Variable(const char* const name, char var);

	/// Dispatches the name and value of a signed char variable.
	Assert& Variable(const char* const name, signed char var);

	/// Dispatches the name and value of an unsigned char variable.
	Assert& Variable(const char* const name, unsigned char var);

	/// Dispatches the name and value of a short variable.
	Assert& Variable(const char* const name, short var);

	/// Dispatches the name and value of an unsigned short variable.
	Assert& Variable(const char* const name, unsigned short var);

	/// Dispatches the name and value of an int variable.
	Assert& Variable(const char* const name, int var);

	/// Dispatches the name and value of an unsigned int variable.
	Assert& Variable(const char* const name, unsigned int var);

	/// Dispatches the name and value of a long variable.
	Assert& Variable(const char* const name, long var);

	/// Dispatches the name and value of an unsigned long variable.
	Assert& Variable(const char* const name, unsigned long var);

	/// Dispatches the name and value of a long long variable.
	Assert& Variable(const char* const name, long long var);

	/// Dispatches the name and value of an unsigned long long variable.
	Assert& Variable(const char* const name, unsigned long long var);

	/// Dispatches the name and value of a float variable.
	Assert& Variable(const char* const name, float var);

	/// Dispatches the name and value of a double variable.
	Assert& Variable(const char* const name, double var);

	/// Dispatches the name and value of a string literal/string.
	Assert& Variable(const char* const name, const char* const var);

	/// Dispatches the name and value of a generic pointer type.
	template <typename T>
	Assert& Variable(const char* const name, T* const var);

	/// Dispatches the name and value of a generic pointer type.
	template <typename T>
	Assert& Variable(const char* const name, const T* const var);

	/// Dispatches the name and value of a generic type.
	template <typename T, class = typename std::enable_if< std::is_enum<T>::value >::type>
	Assert& Variable(const char* const name, const T var);

	/// Dispatches the name and value of a generic type.
	template <typename T, class = typename std::enable_if< !std::is_enum<T>::value >::type>
	Assert& Variable(const char* const name, const T& var);

private:
	/// Dispatches the name and value of a generic type to registered assertion handlers and loggers.
	static void Dispatch(const SourceInfo& sourceInfo, const char* const name, const char* format, ...);
	static void DispatchInternal(const SourceInfo& sourceInfo, const char* const name, const char* pValue);

	X_NO_COPY(Assert);
	X_NO_ASSIGN(Assert);

	const SourceInfo& sourceInfo_;
};

#include "Assert.inl"

X_NAMESPACE_END


#endif // X_ASSERT_H_
