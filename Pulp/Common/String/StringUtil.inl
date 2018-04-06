

namespace strUtil
{
    template<typename T>
    inline size_t StringBytes(const T& str)
    {
        return str.length() * sizeof(typename T::value_type);
    }

    template<typename T>
    inline size_t StringBytesIncNull(const T& str)
    {
        return (str.length() + 1) * sizeof(typename T::value_type);
    }

    inline bool IsWhitespace(const char character)
    {
        return ((character == 32) || ((character >= 9) && (character <= 13)));
    }

    inline bool IsWhitespaceW(const wchar_t character)
    {
        return ((character == 32) || ((character >= 9) && (character <= 13)));
    }

    inline bool IsAlphaNum(const char c)
    {
        return c >= -1 && isalnum(static_cast<int>(c)) != 0;
    }

    inline bool IsAlphaNum(const uint8_t c)
    {
        return isalnum(static_cast<int>(c)) != 0;
    }

    inline bool IsAlpha(const char c)
    {
        return c >= -1 && isalpha(static_cast<int>(c)) != 0;
    }

    inline bool IsAlpha(const uint8_t c)
    {
        return isalpha(static_cast<int>(c)) != 0;
    }

    template<typename CharT>
    inline bool IsDigit(const CharT character)
    {
        return ((character >= '0') && (character <= '9'));
    }

    inline bool IsDigitW(const wchar_t character)
    {
        return ((character >= '0') && (character <= '9'));
    }

    template<typename CharT>
    inline bool IsNumeric(const CharT* str)
    {
        if (*str == '-') {
            str++;
        }

        bool dotFnd = false;

        for (size_t i = 0; str[i]; i++) {
            if (!IsDigit(str[i])) {
                if (str[i] == '.' && !dotFnd) {
                    dotFnd = true;
                }
                else {
                    return false;
                }
            }
        }

        return true;
    }

    template<typename CharT>
    inline bool IsNumeric(const CharT* startInclusive, const CharT* endExclusive)
    {
        X_ASSERT(endExclusive > startInclusive, "string can't be empty")
        (startInclusive, endExclusive);

        if (*startInclusive == '-') {
            startInclusive++;
        }

        bool dotFnd = false;

        while (startInclusive < endExclusive) {
            if (!IsDigit(*startInclusive)) {
                if (*startInclusive == '.' && !dotFnd) {
                    dotFnd = true;
                }
                else {
                    return false;
                }
            }

            ++startInclusive;
        }

        return true;
    }

    template<size_t N>
    inline const char* Convert(const wchar_t* input, char (&output)[N])
    {
        return Convert(input, output, N);
    }

    template<size_t N>
    inline const wchar_t* Convert(const char* input, wchar_t (&output)[N])
    {
        return Convert(input, output, N * 2);
    }

    template<typename T>
    inline T StringToInt(const char* str)
    {
        return safe_static_cast<T>(atoi(str));
    }

    template<typename T>
    inline T StringToInt(const wchar_t* str)
    {
        return safe_static_cast<T>(_wtoi(str));
    }

    template<typename T>
    inline T StringToInt(const char* str, int32_t base)
    {
        char* pEnd;
        return safe_static_cast<T>(strtol(str, &pEnd, base));
    }

    template<typename T>
    inline T StringToInt(const wchar_t* str, int32_t base)
    {
        wchar_t* pEnd;
        return safe_static_cast<T>(wcstol(str, &pEnd, base));
    }

    template<typename T>
    inline T StringToInt(const char* str, const char** pEndPtr, int32_t base)
    {
        X_ASSERT_NOT_NULL(pEndPtr);
        return safe_static_cast<T>(strtol(str, const_cast<char**>(pEndPtr), base));
    }

    template<typename T>
    inline T StringToInt(const wchar_t* str, const wchar_t** pEndPtr, int32_t base)
    {
        X_ASSERT_NOT_NULL(pEndPtr);
        return safe_static_cast<T>(wcstol(str, const_cast<wchar_t**>(pEndPtr), base));
    }

    template<typename T>
    inline T StringToInt(const char* startInclusive, const char* endExclusive)
    {
        // skip any white space till either digit '+', '-'
        while (startInclusive < endExclusive && IsWhitespace(*startInclusive)) {
            ++startInclusive;
        }

        if (startInclusive == endExclusive) {
            return 0;
        }

        char sign = *startInclusive;
        if (sign == '-' || sign == '+') {
            ++startInclusive;
        }

        int val = 0;
        while (startInclusive < endExclusive) {
            char ch = (*startInclusive++);
            if (!IsDigit(ch)) {
                break;
            }

            ch -= '0';
            val = val * 10 - ch;
        }

        if (sign != '-') {
            val = -val;
        }

        return safe_static_cast<T>(val);
    }

    template<typename T>
    inline T StringToInt(const wchar_t* startInclusive, const wchar_t* endExclusive)
    {
        while (startInclusive < endExclusive && IsWhitespaceW(*startInclusive)) {
            ++startInclusive;
        }

        if (startInclusive == endExclusive) {
            return 0;
        }

        wchar_t sign = *startInclusive;
        if (sign == '-' || sign == '+') {
            ++startInclusive;
        }

        int val = 0;
        while (startInclusive < endExclusive) {
            wchar_t ch = (*startInclusive++);
            if (!IsDigitW(ch)) {
                break;
            }

            ch -= '0';
            val = val * 10 - ch;
        }

        if (sign != '-') {
            val = -val;
        }

        return safe_static_cast<T>(val);
    }

    template<typename T>
    inline T StringToFloat(const char* str)
    {
        return static_cast<T>(atof(str));
    }

    template<typename T>
    inline T StringToFloat(const wchar_t* str)
    {
        return static_cast<T>(_wtof(str));
    }

    template<>
    inline float StringToFloat(const char* str, const char** pEndPtr)
    {
        return strtof(str, const_cast<char**>(pEndPtr));
    }

    template<>
    inline double StringToFloat(const char* str, const char** pEndPtr)
    {
        return strtod(str, const_cast<char**>(pEndPtr));
    }

    template<>
    inline float StringToFloat(const wchar_t* str, const wchar_t** pEndPtr)
    {
        return wcstof(str, const_cast<wchar_t**>(pEndPtr));
    }

    template<>
    inline double StringToFloat(const wchar_t* str, const wchar_t** pEndPtr)
    {
        return wcstod(str, const_cast<wchar_t**>(pEndPtr));
    }
} // namespace strUtil
