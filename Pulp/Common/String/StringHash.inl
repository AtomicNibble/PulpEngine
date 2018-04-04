

namespace internal
{
	template <unsigned int N, unsigned int I>
	struct StringHashGenerator
	{
		inline constexpr static StrHash::Type Hash(const char(&str)[N]) 
		{ 
			return (StringHashGenerator<N, I-1>::Hash(str) ^ str[I-1])*16777619u;
		}
	};


	template <unsigned int N>
	struct StringHashGenerator<N, 1>
	{
		inline constexpr static StrHash::Type Hash(const char(&str)[N])
		{
			return (2166136261u ^ str[0])*16777619u;
		}
	};


	template <unsigned int N>
	struct StringHashGenerator<N, 0>
	{
		inline constexpr static StrHash::Type Hash(const char(&str)[N])
		{
#if X_COMPILER_CLANG == 0
			static_assert(false, "Empty constant strings cannot be hashed.");
#endif // !X_COMPILER_CLANG
			return 0u;
		}
	};


	typedef compileTime::IntToType<false> NonStringLiteral;

	typedef compileTime::IntToType<true> StringLiteral;


	template <size_t N>
	inline constexpr StrHash::Type GenerateHash(const char(&str)[N], StringLiteral)
	{
		// defer the hash generation to the template mechanism
		return StringHashGenerator<N, N-1>::Hash(str);
	}


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
X_INLINE StrHash::StrHash(const StrHash& oth) 
	
= default;

// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
X_INLINE StrHash::StrHash(const T& str) :
	hash_(
		X_NAMESPACE(core)::internal::GenerateHash(str, compileTime::IntToType<compileTime::IsStringLiteral<const T&>::Value>())
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
