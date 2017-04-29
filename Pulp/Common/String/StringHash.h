#pragma once

#ifndef _H_STRING_HASH_
#define _H_STRING_HASH_

#include <CompileTime/IntToType.h>
#include <CompileTime/IsStringLiteral.h>
#include <Hashing/Fnva1Hash.h>


X_NAMESPACE_BEGIN(core)


/// \class xStrHash
/// \brief A class representing a hashed string bitches
/// \detail Creates a hashed string from string literals/ constant string and dynamic strings
/// string literals will hashed at compile time.
/// 
/// The underlying hashing algorithm is FNV-a1 http://isthe.com/chongo/tech/comp/fnv/#FNV-1a
/// \Examples
///   // directly compiled into a hash, no hashing at run-time
///   core::xStrHash hs("Test");
///
///   // hash calculated at run-time
///   std::xStr s2("Test");
///   core::xStrHash hs2(s2.c_str());
///
///   // directly compiled into a hash, no hashing at run-time
///   char t[] = "Test";
///   core::xStrHash hs3(t);
///
///   // hash calculated at run-time because the type is const char*, pointing to a string literal in read-only memory
///   const char* t2 = "Test";
///   core::xStrHash hs4(t2);
class StrHash
{
public:
	typedef uint32_t Type;

	X_INLINE StrHash();

	/// \brief Constructs a xStrHash from either a constant string/string literal, or a const char*.
	/// \details The generation of the hash is deferred to the proper function using a compile-time dispatch mechanism,
	/// based on the compile-time result of IsStringLiteral.
	/// \remark The constructor is non-explicit on purpose.
	template <typename T>
	X_INLINE StrHash(const T& str);
	
	X_INLINE StrHash(const char* str, size_t length);	/// Constructs a StringHash from a string with a certain length.

	X_INLINE operator Type(void) const;	/// Cast operator, returning the string's hash.

private:
	Type hash_;
};

#include "StringHash.inl"

X_NAMESPACE_END

#endif // _H_STRING_HASH_