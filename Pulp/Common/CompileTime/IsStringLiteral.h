#pragma once

#ifndef X_ISSTRINGLITERAL_H_
#define X_ISSTRINGLITERAL_H_


X_NAMESPACE_BEGIN(core)

namespace compileTime
{
	/// \ingroup CompileTime
	/// \brief Determines at compile-time whether a given type is a string literal.
	/// \details Knowing if a type is a string literal is used by the quasi-compile time string hashing mechanism in
	/// order to hash string literals using simple template-metaprogramming. By default, all types are considered
	/// to be non-string literals.
	/// \sa StringHash
	template <typename T>
	struct IsStringLiteral
	{
		static const bool Value = false;
	}; 


	/// \ingroup CompileTime
	/// \brief Partial template specialization for const char arrays/string literals.
	/// \details This template specialization yields true for all arrays of constant characters, such as "Hello World".
	/// \sa StringHash
	template <size_t N>
	struct IsStringLiteral<const char [N]>
	{
		static const bool Value = true;
	};

	template <size_t N>
	struct IsStringLiteral<const char(&)[N]>
	{
		static const bool Value = true;
	};

}

X_NAMESPACE_END


#endif
