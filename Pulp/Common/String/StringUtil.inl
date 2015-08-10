



namespace strUtil
{

	inline bool IsWhitespace(char character)
	{
		return ((character == 32) || ((character >= 9) && (character <= 13)));
	}

	inline bool IsWhitespaceW(wchar_t character)
	{
		return ((character == 32) || ((character >= 9) && (character <= 13)));
	}

	inline bool IsAlphaNum(char c)
	{
		return c >= -1 && isalnum(static_cast<int>(c)) != 0;
	}

	inline bool IsAlphaNum(uint8_t c)
	{
		return isalnum(static_cast<int>(c)) != 0;
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

	template <size_t N>
	inline const wchar_t* Convert(const char* input, wchar_t(&output)[N])
	{
		return Convert(input, output, N  * 2);
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
