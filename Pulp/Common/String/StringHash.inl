

namespace internal
{
	/// \brief Base template for generating a single hash value for a string literal of size N at position I.
	template <unsigned int N, unsigned int I>
	struct StringHashGenerator
	{
		/// Function generating the hash value for a string literal by calling itself recursively via other StringHash
		/// template parameters.
		inline constexpr static StrHash::Type Hash(const char(&str)[N]) 
		{ 
			return (StringHashGenerator<N, I-1>::Hash(str) ^ str[I-1])*16777619u;
		}
	};


	/// \brief Partial template specialization ending the recursion of Hash() function calls.
	template <unsigned int N>
	struct StringHashGenerator<N, 1>
	{
		/// Function generating the hash value for the first character in the given string literal.
		inline constexpr static StrHash::Type Hash(const char(&str)[N])
		{
			return (2166136261u ^ str[0])*16777619u;
		}
	};


	/// \brief Partial template specialization prohibiting the hashing of empty string literals.
	template <unsigned int N>
	struct StringHashGenerator<N, 0>
	{
		/// Function prohibiting the hashing of empty string literals.
		inline constexpr static StrHash::Type Hash(const char(&str)[N])
		{
#if X_COMPILER_CLANG == 0
			static_assert(false, "Empty constant strings cannot be hashed.");
#endif // !X_COMPILER_CLANG
			return 0u;
		}
	};


	/// Type that represents the outcome of a compile-time value for any other type than a string literal.
	typedef compileTime::IntToType<false> NonStringLiteral;

	/// Type that represents the outcome of a compile-time value for a string literal type.
	typedef compileTime::IntToType<true> StringLiteral;


	/// \brief Generates a hash for constant strings/string literals.
	/// \remark The StringLiteral argument is never used, and serves only as a type used in overload resolution
	/// in the type-based dispatch mechanism.
	template <size_t N>
	inline constexpr StrHash::Type GenerateHash(const char(&str)[N], StringLiteral)
	{
		// defer the hash generation to the template mechanism
		return StringHashGenerator<N, N-1>::Hash(str);
	}


	/// \brief Generates a hash for non-constant strings.
	/// \remark The NonStringLiteral argument is never used, and serves only as a type used in overload resolution
	/// in the type-based dispatch mechanism.
	inline StrHash::Type GenerateHash(const char* pStr, NonStringLiteral)
	{
		return Hash::Fnv1aHash(pStr, strUtil::strlen(pStr));
	}
}



X_INLINE StrHash::StrHash() :
	hash_((Type)-1)
{

}

// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
X_INLINE StrHash::StrHash(const T& str) :
	hash_(
		X_NAMESPACE(core)::internal::GenerateHash(str, compileTime::IntToType<compileTime::IsStringLiteral<T>::Value>())
	)
{

}


// ---------------------------------------------------------------------------------------------------------------------
X_INLINE StrHash::StrHash(const char* pStr, size_t length) :
	hash_(
		Hash::Fnv1aHash(pStr, length)
	)
{
}

// ---------------------------------------------------------------------------------------------------------------------
X_INLINE StrHash::StrHash(const char* pBegin, const char* pEnd) :
	hash_(
		Hash::Fnv1aHash(pBegin, safe_static_cast<size_t>(pEnd - pBegin))
	)
{

}

// ---------------------------------------------------------------------------------------------------------------------
X_INLINE StrHash::StrHash(Type hash) :
	hash_(hash)
{

}


// ---------------------------------------------------------------------------------------------------------------------
X_INLINE StrHash::operator Type(void) const
{
	return hash_;
}
