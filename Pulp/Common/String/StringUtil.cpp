#include "EngineCommon.h"
#include "StringUtil.h"
#include "Path.h"

#include "StrRef.h"	
#include <string>

#include <direct.h>

X_NAMESPACE_BEGIN(core)


namespace strUtil
{
	template<>
	inline size_t StringBytes(const std::string& str)
	{
		return str.length();
	}

	template<>
	inline size_t StringBytes(const std::wstring& str)
	{
		return str.length() * 2;
	}

	template<>
	inline size_t StringBytes(const core::StringRef<char>& str)
	{
		return str.length();
	}

	template<>
	inline size_t StringBytes(const core::StringRef<wchar_t>& str)
	{
		return str.length() * 2;
	}

	template<>
	inline size_t StringBytesIncNull(const std::string& str)
	{
		return str.length() + 1;
	}

	template<>
	inline size_t StringBytesIncNull(const std::wstring& str)
	{
		return (str.length() + 1) * 2;
	}

	template<>
	inline size_t StringBytesIncNull(const core::StringRef<char>& str)
	{
		return str.length() + 1;
	}

	template<>
	inline size_t StringBytesIncNull(const core::StringRef<wchar_t>& str)
	{
		return (str.length() + 1) * 2;
	}

	static const size_t initerlizer_1 = StringBytes(std::string(""));
	static const size_t initerlizer_2 = StringBytes(std::wstring(L""));
	static const size_t initerlizer_3 = StringBytes(core::StringRef<char>(""));
	static const size_t initerlizer_4 = StringBytes(core::StringRef<wchar_t>(L""));
	static const size_t initerlizer_5 = StringBytesIncNull(std::string(""));
	static const size_t initerlizer_6 = StringBytesIncNull(std::wstring(L""));
	static const size_t initerlizer_7 = StringBytesIncNull(core::StringRef<char>(""));
	static const size_t initerlizer_8 = StringBytesIncNull(core::StringRef<wchar_t>(L""));

	namespace
	{
#define  bswap16	_byteswap_ushort
#define  bswap32	_byteswap_ulong
#define  MSBF16(x)   bswap16( (*(uint16_t const __declspec(align(1))*)x) )
#define  MSBF32(x)   bswap32( (*(uint32_t const __declspec(align(1))*)x) )

#define  compxm(a,b) _mm_movemask_epi8(_mm_cmpeq_epi8((a), (b)))
#define  xmload(p)   _mm_load_si128((__m128i const *)(p))
#define  load16(p)   (*(uint16_t const*)(p))
#define  load32(p)   (*(uint32_t const*)(p))

		char const*scanstr2(char const *tgt, char const pat[2])
		{
			__m128i const   zero = _mm_setzero_si128();
			__m128i const   p0 = _mm_set1_epi8(pat[0]);
			__m128i const   p1 = _mm_set1_epi8(pat[1]);
			unsigned        f = 15 & (intptr_t)tgt;
			uint16_t        pair = load16(pat);
			if (f) {
				__m128i  x = xmload(tgt - f);
				unsigned u = compxm(zero, x) >> f;
				unsigned v = ((compxm(p0, x) & (compxm(p1, x) >> 1)) >> f) & ~u & (u - 1);
				if (v) return tgt + bitUtil::ScanBitsForward(v) - 1;
				if (u) return  NULL;
				tgt += 16 - f;
				if (load16(tgt - 1) == pair)
					return tgt - 1;
			}
			while (1) {
				__m128i  x = xmload(tgt);
				unsigned u = compxm(zero, x);
				unsigned v = compxm(p0, x) & (compxm(p1, x) >> 1) & ~u & (u - 1);
				if (v) return tgt + bitUtil::ScanBitsForward(v);
				if (u) return  NULL;
				tgt += 16;
				if (load16(tgt - 1) == pair)
					return tgt - 1;
			}
		}

		char const *scanstr3(char const *tgt, char const pat[3])
		{
			__m128i const   zero = _mm_setzero_si128();
			__m128i const   p0 = _mm_set1_epi8(pat[0]);
			__m128i const   p1 = _mm_set1_epi8(pat[1]);
			__m128i const   p2 = _mm_set1_epi8(pat[2]);
			unsigned        trio = load32(pat) & 0x00FFFFFF;
			unsigned        f = 15 & (uintptr_t)tgt;

			if (f) {
				__m128i  x = xmload(tgt);
				unsigned u = compxm(zero, x);
				unsigned v = compxm(p0, x) & (compxm(p1, x) >> 1);
				v = (v & (compxm(p2, x) >> 2) & ~u & (u - 1)) >> f;
				if (v) return tgt + bitUtil::ScanBitsForward(v) - 1;
				tgt += 16 - f;
				v = load32(tgt - 2);
				if (trio == (v & 0x00FFFFFF)) return tgt - 2;
				if (trio == v >> 8) return tgt - 1;
				if (u >> f) return  NULL;
			}

			while (1) {
				__m128i  x = xmload(tgt);
				unsigned u = compxm(zero, x);
				unsigned v = compxm(p0, x) & (compxm(p1, x) >> 1);
				v = (v & (compxm(p2, x) >> 2) & ~u & (u - 1)) >> f;
				if (v) return tgt + bitUtil::ScanBitsForward(v) - 1;
				tgt += 16;
				v = load32(tgt - 2);
				if (trio == (v & 0x00FFFFFF)) return tgt - 2;
				if (trio == v >> 8) return tgt - 1;
				if (u) return  NULL;
			}
		}

		char const* scanstrm(char const *tgt, char const *pat, uint32_t len)
		{
			uint32_t head = MSBF32(pat), wind = 0, next;

			pat += 4, len -= 4;
			while ((next = *(uint8_t const*)tgt++)) {
				wind = (wind << 8) + next;
				if (wind == head && !memcmp(tgt, pat, len))
					return tgt - 4;
			}
			return  NULL;
		}

		// only used for function below.
		uint32_t replaceAll(const char* startInclusive, const char* endExclusive, const char original, const char replacement)
		{
			// find me baby
			uint32_t count = 0;
			char* start = (char*)startInclusive;
			while (start < endExclusive) {
				if (*start == original) {
					*start = replacement;
					count++;
				}
				++start;
			}
			return count;
		}

		void ReplaceSlashes(const char* path, const char* pathEnd)
		{
			replaceAll(path, pathEnd,
				Path<char>::NON_NATIVE_SLASH, Path<char>::NATIVE_SLASH);
		}


		template <size_t N>
		struct Implementation {};

		/// Template specialization for 4-byte types.
		template <>
		struct Implementation<8u>
		{
			static size_t strlen(const char* str)
			{
				__m128i zero = _mm_set1_epi8(0);
				__m128i *s_aligned = (__m128i*) (((uint64_t)str) & -0x10L);
				uint8_t misbits = (uint8_t)(((uint64_t)str) & 0xf);
				__m128i s16cs = _mm_load_si128(s_aligned);
				__m128i bytemask = _mm_cmpeq_epi8(s16cs, zero);
				int bitmask = _mm_movemask_epi8(bytemask);
				bitmask = (bitmask >> misbits) << misbits;

				// Alternative: use TEST instead of BSF, then BSF at end (only). Much better on older CPUs
				// TEST has latency 1, while BSF has 3!
				while (bitmask == 0) {
					s16cs = _mm_load_si128(++s_aligned);
					bytemask = _mm_cmpeq_epi8(s16cs, zero);
					bitmask = _mm_movemask_epi8(bytemask);
				}

				return (((const char*)s_aligned) - str) + (size_t)bitUtil::ScanBitsForward(bitmask);
			}
		};

		template <>
		struct Implementation<4u>
		{
#if X_64 == 0
			static size_t strlen(const char* str)
			{
				__m128i zero = _mm_set1_epi8(0);
				__m128i *s_aligned = (__m128i*) (((long)str) & -0x10L);
				uint8_t misbits = (uint8_t)(((long)str) & 0xf);
				__m128i s16cs = _mm_load_si128(s_aligned);
				__m128i bytemask = _mm_cmpeq_epi8(s16cs, zero);
				int bitmask = _mm_movemask_epi8(bytemask);
				bitmask = (bitmask >> misbits) << misbits;

				while (bitmask == 0) {
					s16cs = _mm_load_si128(++s_aligned);
					bytemask = _mm_cmpeq_epi8(s16cs, zero);
					bitmask = _mm_movemask_epi8(bytemask);
				}
				return (((const char*)s_aligned) - str) + bitUtil::ScanBitsForward(bitmask);
			}
#endif
		};


		uint32_t upperCaseSIMD(uint32_t x)
		{
			uint32_t b = 0x80808080ul | x;
			uint32_t c = b - 0x61616161ul;
			uint32_t d = ~(b - 0x7b7b7b7bul);
			uint32_t e = (c & d) & (~x & 0x80808080ul);
			return x - (e >> 2);
		}

	} // namespace

	uint32_t strlen(const char* str)
	{
		return static_cast<uint32_t>(Implementation<sizeof(const char*)>::strlen(str));
	}

	uint32_t strlen(const wchar_t* str)
	{
		return static_cast<uint32_t>(wcslen(str));
	}

	const char* Convert(const wchar_t *input, char *output, uint32_t outputLength)
	{
		size_t convertedChars = 0;
		wcstombs_s(&convertedChars, output, outputLength, input, _TRUNCATE);
		return output;
	}

	const wchar_t* Convert(const char *input, wchar_t *output, uint32_t outputBytes)
	{
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, output, outputBytes / 2, input, _TRUNCATE);
		return output;
	}


	unsigned int Count(const char* startInclusive, const char* endExclusive, char what)
	{
		uint i;
		const char* Start = startInclusive;

		for (i = 0; Start < endExclusive; ++Start) {
			if (*Start == what)
				++i;
		}

		return i;
	}


	bool IsEqual(const char* str1, const char* str2)
	{
		while (*str1 && (*str1 == *str2))
			str1++, str2++;

		// are they both null ?
		return (*(const uint8_t*)str1 - *(const uint8_t*)str2) == 0;
	}


	/// Returns whether two strings are equal, checks the length of the 1st range.
	bool IsEqual(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2)
	{
		size_t Len = endExclusiveS1 - startInclusiveS1;

		return memcmp(startInclusiveS1, startInclusiveS2, Len) == 0;
	}

	bool IsEqual(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2, const char* endExclusiveS2)
	{
		size_t Len = endExclusiveS1 - startInclusiveS1;

		if (Len == (endExclusiveS2 - startInclusiveS2))
			return memcmp(startInclusiveS1, startInclusiveS2, Len) == 0;

		return false;
	}


	bool IsEqualCaseInsen(const char* str1, const char* str2)
	{
		while (*str1 && (::tolower(*str1) == ::tolower(*str2)))
			str1++, str2++;

		// are they both null ?
		return (*(const uint8_t*)str1 - *(const uint8_t*)str2) == 0;
	}

	/// Returns whether two strings are equal, checks the length of the 1st range.
	bool IsEqualCaseInsen(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2)
	{
		size_t Len = endExclusiveS1 - startInclusiveS1;

		while (Len && *startInclusiveS2 
			&& (::tolower(*startInclusiveS1) == ::tolower(*startInclusiveS2)))
		{
			Len--;
			startInclusiveS1++;
			startInclusiveS2++;
		}

		return Len == 0 && !(*startInclusiveS2);
	}

	bool IsEqualCaseInsen(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2, const char* endExclusiveS2)
	{
		size_t Len = endExclusiveS1 - startInclusiveS1;

		if (Len == (endExclusiveS2 - startInclusiveS2))
		{
			// need to respect length values.
			while (Len && (::tolower(*startInclusiveS1) == ::tolower(*startInclusiveS2))) {
				Len--;
				startInclusiveS1++;
				startInclusiveS2++;
			}
			return Len == 0;
		}

		return false;
	}


	/// \brief Finds the first character in a string that is not a certain character, and returns a pointer to it.
	/// \remark Returns a \c nullptr if the character could not be found.
	const char* FindNon(const char* startInclusive, const char* endExclusive, char what)
	{
		if (startInclusive >= endExclusive)
			return nullptr;

		const char* result = startInclusive;

		while (*result == what)
		{
			++result;
			if (result >= endExclusive)
				return nullptr;
		}

		return result;
	}

	/// \brief Finds a character in a string, and returns a pointer to the last occurrence of the character.
	/// \remark Returns a \c nullptr if the character could not be found.
	const char* FindLast(const char* startInclusive, const char* endExclusive, char what)
	{
		const char* result = endExclusive - 1;

		if (startInclusive >= result)
			return nullptr;

		while (*result != what)
		{
			--result;
			if (startInclusive >= result)
				return nullptr;
		}

		return result;
	}

	/// \brief Finds the last character in a string that is not a certain character, and returns a pointer to it.
	/// \remark Returns a \c nullptr if the character could not be found.
	const char* FindLastNon(const char* startInclusive, const char* endExclusive, char what)
	{
		const char* result = endExclusive - 1;

		if (startInclusive >= result)
			return nullptr;

		while (*result == what)
		{
			--result;
			if (startInclusive >= result)
				return nullptr;
		}

		return result;
	}


	const char* FindLastWhitespace(const char* startInclusive, const char* endExclusive)
	{
		do
		{
			--endExclusive;
			// reached the end ?
			if (endExclusive < startInclusive)
				break;
			// if whitespace return it.
			if (IsWhitespace(*endExclusive))
				return endExclusive;
		} while (1);

		return nullptr;
	}

	const char* FindLastNonWhitespace(const char* startInclusive, const char* endExclusive)
	{
		do
		{
			--endExclusive;
			// reached the end ?
			if (endExclusive < startInclusive)
				break;
			// if not whitespace return it.
			if (!IsWhitespace(*endExclusive))
				return endExclusive;
		} while (1);

		return nullptr;
	}

	const char* FindNonWhitespace(const char* startInclusive, const char* endExclusive)
	{
		while (startInclusive < endExclusive)
		{
			// if not whitespace return it.
			if (!IsWhitespace(*startInclusive))
				return startInclusive;

			++startInclusive;
		}

		return nullptr;
	}


	/// \brief Finds a character in a string, and returns a pointer to the first occurrence of the character.
	/// \remark Returns a \c nullptr if the character could not be found.
	const char* Find(const char* startInclusive, const char* endExclusive, char what)
	{
		while (startInclusive < endExclusive)
		{
			if (*startInclusive == what)
				return startInclusive;
			++startInclusive;
		}

		return nullptr;
	}

	const char* Find(const char* startInclusive, const char* endExclusive, const char* what)
	{
		return Find(startInclusive, endExclusive, what, strlen(what));
	}


	const char* Find(const char* startInclusive, const char* endExclusive, const char* what, uint32_t whatLength)
	{
		// should check how much these aligne ment checks take.
		// might be best to have a seprate aligned find
		// for when i know it's aligned, as the global optermisation attempt.
		// may be slower due to align checks.
		if (core::pointerUtil::IsAligned(startInclusive, 16, 0) &&
			core::pointerUtil::IsAligned(what, 16, 0))
		{

			switch (whatLength) {
				case  0: return startInclusive;
				case  1: return strchr(startInclusive, *what);
				case  2: return scanstr2(startInclusive, what);
				case  3: return scanstr3(startInclusive, what);
				default:
					return scanstrm(startInclusive, what, whatLength);
			}
		}
		return strstr(startInclusive, what);
	}

	const char* Find(const char* startInclusive, const char* endExclusive, const char* whatStart, const char* whatEnd)
	{
		return Find(startInclusive, endExclusive, whatStart, safe_static_cast<int, size_t>(whatEnd - whatStart));
	}


	const char* FindCaseInsensitive(const char* startInclusive, const char* endExclusive, const char* what)
	{
		return FindCaseInsensitive(startInclusive, endExclusive, what, strlen(what));
	}

	const char* FindCaseInsensitive(const char* startInclusive, const char* what)
	{
		return FindCaseInsensitive(startInclusive, startInclusive + strlen(startInclusive), what, strlen(what));
	}

	const char* FindCaseInsensitive(const char* startInclusive, const char* endExclusive, const char* what, size_t whatLength)
	{
		size_t len = endExclusive - startInclusive;

		if (whatLength > len)
			return nullptr;
		
		while (startInclusive < endExclusive)
		{
			// we check if we have a match.
			// we need to match the whole string.
			if (upperCaseSIMD(*startInclusive) == upperCaseSIMD(*what))
			{
				// we know the substring fits.
				// so we just want to know if we have the rest of the substring.
				const char* current = startInclusive + 1;
				const char* currentWhat = what + 1;
				size_t i = 0;
				for (; i < (whatLength-1); i++)
				{
					if (upperCaseSIMD(*current) != upperCaseSIMD(*currentWhat))
						break;

					++current;
					++currentWhat;
				}

				if (i == (whatLength - 1))
					return startInclusive;
			}

			if (len == whatLength) // sub string can't fit anymore.
				return nullptr;

			--len;
			++startInclusive;
		}

		return nullptr;
	}

	const char* FindCaseInsensitive(const char* startInclusive, char what)
	{
		return FindCaseInsensitive(startInclusive, startInclusive + strlen(startInclusive), what);
	}

	const char* FindCaseInsensitive(const char* startInclusive, const char* endExclusive, char what)
	{
		size_t len = endExclusive - startInclusive;

		if (len == 0)
			return nullptr;

		char upperWhat = upperCaseSIMD(what);

		while (startInclusive < endExclusive)
		{
			if (upperCaseSIMD(*startInclusive) == upperWhat)
			{
				return startInclusive;
			}

			++startInclusive;
		}

		return nullptr;
	}

	const char* workingDir(WorkingDirStr& buf)
	{
		_getcwd(buf, sizeof(buf)-1);
		buf[MAX_PATH - 1] = 0;

		return buf;
	}

	bool HasFileExtension(const char* path)
	{
		return FileExtension(path) != nullptr;
	}

	bool HasFileExtension(const char* startInclusive, const char* endExclusive)
	{
		return FileExtension(startInclusive, endExclusive) != nullptr;
	}

	const char* FileExtension(const char* path)
	{
		return FileExtension(path, path + strUtil::strlen(path));
	}
	const char* FileExtension(const char* startInclusive, const char* endExclusive)
	{
		const char* res = strUtil::FindLast(startInclusive, endExclusive, '.');
		// I think it might be better to return null here, instead of start.
		if (!res || res == (endExclusive - 1))
			return nullptr;
		return res + 1;
	}

	const char* FileName(const char* path)
	{
		return FileName(path, path + strUtil::strlen(path));
	}



	const char* FileName(const char* startInclusive, const char* endExclusive)
	{
		// make sure slash is correct.
		strUtil::ReplaceSlashes(startInclusive, endExclusive);

		const char* res = strUtil::FindLast(startInclusive, endExclusive, Path<char>::NATIVE_SLASH);

		if (!res || res == (endExclusive-1))
			return startInclusive; // might just be file name.
		return res + 1;
	}

	bool WildCompare(const char* wild, const char* string)
	{
		const char *cp = nullptr, *mp = nullptr;

		while ((*string) && (*wild != '*')) {
			if ((*wild != *string) && (*wild != '?')) {
				return 0;
			}
			wild++;
			string++;
		}

		while (*string) 
		{
			if (*wild == '*') {
				if (!*++wild) {
					return 1;
				}
				mp = wild;
				cp = string + 1;
			}
			else if ((*wild == *string) || (*wild == '?')) {
				wild++;
				string++;
			}
			else {
				wild = mp;
				string = cp++;
			}
		}

		while (*wild == '*') {
			wild++;
		}
		return !*wild;
	}


}


X_NAMESPACE_END