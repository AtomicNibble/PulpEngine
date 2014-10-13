



namespace strUtil
{
	namespace internal
	{
		template <size_t N>
		struct Implementation {};

		/// Template specialization for 4-byte types.
		template <>
		struct Implementation<8u>
		{
			static size_t strlen(const char* str)
			{
				__m128i zero = _mm_set1_epi8( 0 );
				__m128i *s_aligned = (__m128i*) (((long long)str) & -0x10L);
				uint8_t misbits = (uint8_t)(((long long)str) & 0xf);
				__m128i s16cs = _mm_load_si128( s_aligned );
				__m128i bytemask = _mm_cmpeq_epi8( s16cs, zero );
				int bitmask = _mm_movemask_epi8( bytemask );
				bitmask = (bitmask>>misbits)<<misbits;

				// Alternative: use TEST instead of BSF, then BSF at end (only). Much better on older CPUs
				// TEST has latency 1, while BSF has 3!
				while (bitmask==0) {
					s16cs = _mm_load_si128( ++s_aligned );
					bytemask = _mm_cmpeq_epi8( s16cs, zero );
					bitmask = _mm_movemask_epi8( bytemask );
				}

				return ((( const char* ) s_aligned ) - str) + (size_t)bitUtil::ScanBitsForward(bitmask);
			}			
		};

		template <>
		struct Implementation<4u>
		{
#if X_64 == 0
			static size_t strlen(const char* str)
			{
				__m128i zero = _mm_set1_epi8( 0 );
				__m128i *s_aligned = (__m128i*) (((long)str) & -0x10L);
				uint8_t misbits = ((long)str) & 0xf;
				__m128i s16cs = _mm_load_si128( s_aligned );
				__m128i bytemask = _mm_cmpeq_epi8( s16cs, zero );
				int bitmask = _mm_movemask_epi8( bytemask );
				bitmask = (bitmask>>misbits)<<misbits;

				while (bitmask==0) {
					s16cs = _mm_load_si128( ++s_aligned );
					bytemask = _mm_cmpeq_epi8( s16cs, zero );
					bitmask = _mm_movemask_epi8( bytemask );
				}
				return ((( const char* ) s_aligned ) - str) + bitUtil::ScanBitsForward(bitmask);
			}	
#endif
		};
	}


	inline uint32_t strlen(const char* str)
	{
		return static_cast<uint32_t>(internal::Implementation<sizeof(const char*)>::strlen(str));
	}

	inline bool IsWhitespace(char character)
	{
		return ((character == 32) || ((character >= 9) && (character <= 13)));
	}

	inline bool IsWhitespaceW(wchar_t character)
	{
		return ((character == 32) || ((character >= 9) && (character <= 13)));
	}

	inline bool IsDigit(char character)
	{
		return ((character >= '0') && (character <= '9'));
	}

	inline bool IsDigitW(wchar_t character)
	{
		return ((character >= '0') && (character <= '9'));
	}


	inline bool IsNumeric(const char* str)
	{
		size_t	i;
		bool	dot;

		if ( *str == '-' ) {
			str++;
		}

		dot = false;
		for ( i = 0; str[i]; i++ ) {
			if ( !IsDigit( str[i] ) ) {
				if ( ( str[ i ] == '.' ) && !dot ) {
					dot = true;
					continue;
				}
				return false;
			}
		}
		return true;
	}


	template <size_t N>
	inline const char* Convert(const wchar_t* input, char (&output)[N])
	{
		return Convert(input, output, N);
	}


	template <typename T>
	inline T StringToInt(const char* str)
	{
		return safe_static_cast<T>(atoi(str));
	}

	template <typename T>
	inline T StringToFloat(const char* str)
	{
		return static_cast<T>(atof(str));
	}
}
