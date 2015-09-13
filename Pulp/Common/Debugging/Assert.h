#pragma once

#ifndef X_ASSERT_H_
#define X_ASSERT_H_

#include <Util/SourceInfo.h>

X_NAMESPACE_BEGIN(core)

/// \ingroup Debugging
/// \class Assert
/// \brief Provides an improved assertion facility.
/// \details There are a few bits missing in the standard assert(), such as outputting a formatted message to the user,
/// halting the debugger at the exact line the assertion statement was written, and outputting individual variables
/// (and their values) used in the assertion condition.
///
/// This custom assert implementation tackles the above-mentioned shortcomings by implementing an assert class
/// in conjunction with custom macros. The assert class itself is responsible for forwarding source-code info
/// (such as file name, line number and function name) and the formatted message to all registered assertion handlers
/// and loggers. The macros allow users to write custom assert statements in a clear and concise way, not having to
/// deal with internal implementation details.
///
/// Individual assertion handlers are registered/unregistered at the assertionDispatch, which takes care of forwarding
/// asserts being fired to all registered handlers. Similarly, all fired assertions will also be forwarded to the
/// registered loggers using the logDispatch.
///
/// Internally, the assertion facility builds upon the C++ idiom of creating a temporary object, calling an infinite
/// number of methods on this object, as in the following example:
/// \code
///   Assert(sourceInfo, "Custom assert message involving two variables").Variable("var1", var1).Variable("var2", var2);
/// \endcode
/// Because a call to Assert::Variable() returns a reference to itself, any number of calls to this method can be chained
/// together. Each call to Assert::Variable() will dispatch the call to all registered assertion handlers and loggers,
/// which in turn can e.g. output the variable's name and its value to the console, to a file, etc.
///
/// The assert supports dispatching variables of all built-in types, as well as custom user types. User types are
/// supported by providing a template specialization to the following method:
/// \code
///   template <typename T>
///   Assert& Variable(const char* const name, const T& var);
/// \endcode
/// For a thorough explanation of how the assertion system works internally,
/// see http://www.altdevblogaday.com/2011/10/12/upgrading-assert-using-the-preprocessor/.
/// \remark This class is not intended to be instantiated by the user, but is used by the \ref X_ASSERT macro, and other
/// convenience macros for asserting certain conditions. The user should always use one of the available \ref X_ASSERT
/// macros.
/// \sa assertionDispatch logDispatch X_ASSERT X_ASSERT_NOT_NULL X_ASSERT_UNREACHABLE X_ASSERT_NOT_IMPLEMENTED X_ASSERT_ALIGNMENT
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
	template <typename T>
	Assert& Variable(const char* const name, const T& var);

private:
	/// Dispatches the name and value of a generic type to registered assertion handlers and loggers.
	template <typename T>
	static void Dispatch(const SourceInfo& sourceInfo, const char* format, const char* const name, const T value);

	X_NO_COPY(Assert);
	X_NO_ASSIGN(Assert);

	const SourceInfo& sourceInfo_;
};

#include "Assert.inl"

X_NAMESPACE_END


#endif // X_ASSERT_H_
