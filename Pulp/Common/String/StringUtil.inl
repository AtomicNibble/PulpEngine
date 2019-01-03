

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

    inline constexpr bool IsWhitespace(const char character)
    {
        return ((character == 32) || ((character >= 9) && (character <= 13)));
    }

    inline constexpr bool IsWhitespaceW(const wchar_t character)
    {
        return ((character == 32) || ((character >= 9) && (character <= 13)));
    }

    inline constexpr bool IsAlphaNum(const char c)
    {
        return IsAlpha(c) || IsDigit(c);
    }

    inline constexpr bool IsAlphaNum(const uint8_t c)
    {
        return IsAlpha(c) || IsDigit(c);
    }

    inline constexpr bool IsAlpha(const char c)
    {
        return (c >= 'a' &&  c <= 'z') || (c >= 'A' &&  c <= 'Z');
    }

    inline constexpr bool IsAlpha(const uint8_t c)
    {
        return (c >= 'a' &&  c <= 'z') || (c >= 'A' &&  c <= 'Z');
    }

    inline constexpr bool IsDigit(const char character)
    {
        return ((character >= '0') && (character <= '9'));
    }

    inline constexpr bool IsDigit(const uint8_t character)
    {
        return ((character >= '0') && (character <= '9'));
    }

    inline constexpr bool IsDigit(const wchar_t character)
    {
        return ((character >= '0') && (character <= '9'));
    }

    template<typename CharT>
    inline bool IsNumeric(const CharT* pStr)
    {
        if (*pStr == '-') {
            ++pStr;
        }

        bool dotFnd = false;

        while(*pStr) {
            if (!IsDigit(*pStr)) {
                if (*pStr == '.' && !dotFnd) {
                    dotFnd = true;
                }
                else {
                    return false;
                }
            }

            ++pStr;
        }

        return true;
    }

    template<typename CharT>
    inline bool IsNumeric(const CharT* startInclusive, const CharT* endExclusive)
    {
        X_ASSERT(endExclusive > startInclusive, "string can't be empty")(startInclusive, endExclusive);

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
    inline const char* Convert(const wchar_t* startInclusive, const wchar_t* endExclusive, char(&output)[N])
    {
        return Convert(startInclusive, endExclusive, output, N * sizeof(wchar_t));
    }

    template<size_t N>
    inline const wchar_t* Convert(const char* input, wchar_t (&output)[N])
    {
        return Convert(input, output, N * sizeof(wchar_t));
    }

    template<size_t N>
    inline const wchar_t* Convert(const char* startInclusive, const char* endExclusive, wchar_t(&output)[N])
    {
        return Convert(startInclusive, endExclusive, output, N * sizeof(wchar_t));
    }

    inline const char* Convert(const wchar_t* startInclusive, const wchar_t* endExclusive, char* output, size_t outputBytes)
    {
        size_t bytesOut;
        return Convert(startInclusive, endExclusive, output, outputBytes, bytesOut);
    }

    inline const wchar_t* Convert(const char* startInclusive, const char* endExclusive, wchar_t* output, size_t outputBytes)
    {
        size_t bytesOut;
        return Convert(startInclusive, endExclusive, output, outputBytes, bytesOut);
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
