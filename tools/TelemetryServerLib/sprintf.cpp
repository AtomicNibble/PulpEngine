#include "stdafx.h"
#include "sprintf.h"

X_NAMESPACE_BEGIN(telemetry)


namespace
{
    constexpr int32_t PRINTF_NTOA_BUFFER_SIZE = 32;
    constexpr int32_t PRINTF_FTOA_BUFFER_SIZE = 32;
    
    X_DECLARE_FLAGS(ParseFlag)(
        Zeropad,
        Left,
        Plus,
        Space,
        Hash,
        Uppercase,
        Char,
        Short,
        Long,
        LongLong,
        Precision
    );

    // internal secure strlen
    // \return The length of the string (excluding the terminating 0) limited by 'maxsize'
    inline int32_t _strnlen_s(const char* pStr, int32_t maxsize)
    {
        const char* s;
        for (s = pStr; *s && maxsize--; ++s) {

        }
        return static_cast<int32_t>(s - pStr);
    }

    // internal test if char is a digit (0-9)
    // \return true if char is a digit
    inline bool _is_digit(char ch)
    {
        return (ch >= '0') && (ch <= '9');
    }

    // internal ASCII string to uint32_t conversion
    uint32_t _atoi(const char** pStr)
    {
        uint32_t i = 0U;
        while (_is_digit(**pStr)) {
            i = i * 10U + static_cast<uint32_t>(*((*pStr)++) - '0');
        }
        return i;
    }


    struct ArgDataHelper
    {
        ArgDataHelper(const ArgData& argData) :
            curIdx_(0),
            argData_(argData)
        {
        }

        template<typename T>
        inline T get(void) {
            const uintptr_t* pArgs = reinterpret_cast<const uintptr_t*>(argData_.data);

            X_ASSERT(curIdx_ < argData_.numArgs, "No more args")(curIdx_, argData_.numArgs);

            if constexpr (sizeof(T) == 4) {
                return (T)(pArgs[curIdx_++] & 0xFFFFFFFF);
            }
            else {
                return (T)pArgs[curIdx_++];
            }
        }

        template<>
        inline double get(void) {
            const uintptr_t* pArgs = reinterpret_cast<const uintptr_t*>(argData_.data);
            X_ASSERT(curIdx_ < argData_.numArgs, "No more args")(curIdx_, argData_.numArgs);

            pArgs += curIdx_++;
            const double* pValue = reinterpret_cast<const double*>(pArgs);

            return *pValue;
        }


        inline std::pair<const char*, int32_t> getString(void) {

            const intptr_t offset = get<intptr_t>();

            uint8_t length = *(argData_.data + offset);
            const char* pStr = reinterpret_cast<const char*>(argData_.data + offset + 1);

            return { pStr, length };
        }

    private:
        int32_t curIdx_;
        const ArgData& argData_;
    };


} // namespace



// internal itoa pFormat
void _ntoa_format(SprintfStrBuf& buf, char* tmpBuf, int32_t len, bool negative,
    int32_t base, int32_t prec, int32_t width, uint32_t flags)
{
    const int32_t start_idx = static_cast<int32_t>(buf.length());

    // pad leading zeros
    if (!(flags & ParseFlag::Left)) {
        if (width && (flags & ParseFlag::Zeropad) && (negative || (flags & (ParseFlag::Plus | ParseFlag::Space)))) {
            width--;
        }
        while ((len < prec) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
            tmpBuf[len++] = '0';
        }
        while ((flags & ParseFlag::Zeropad) && (len < width) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
            tmpBuf[len++] = '0';
        }
    }

    // handle hash
    if (flags & ParseFlag::Hash) {
        if (!(flags & ParseFlag::Precision) && len && ((len == prec) || (len == width))) {
            len--;
            if (len && (base == 16)) {
                len--;
            }
        }
        if ((base == 16) && !(flags & ParseFlag::Uppercase) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
            tmpBuf[len++] = 'x';
        }
        else if ((base == 16) && (flags & ParseFlag::Uppercase) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
            tmpBuf[len++] = 'X';
        }
        else if ((base == 2) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
            tmpBuf[len++] = 'b';
        }
        if (len < PRINTF_NTOA_BUFFER_SIZE) {
            tmpBuf[len++] = '0';
        }
    }

    if (len < PRINTF_NTOA_BUFFER_SIZE) {
        if (negative) {
            tmpBuf[len++] = '-';
        }
        else if (flags & ParseFlag::Plus) {
            tmpBuf[len++] = '+'; // ignore the space if the '+' exists
        }
        else if (flags & ParseFlag::Space) {
            tmpBuf[len++] = ' ';
        }
    }

    // pad spaces up to given width
    if (!(flags & ParseFlag::Left) && !(flags & ParseFlag::Zeropad)) {
        for (int32_t i = len; i < width; i++) {
            buf.append(' ');
        }
    }

    // reverse string
    for (int32_t i = 0; i < len; i++) {
        buf.append(tmpBuf[len - i - 1]);
    }

    // append pad spaces up to given width
    if (flags & ParseFlag::Left) {
        while (static_cast<int32_t>(buf.length()) - start_idx < width) {
            buf.append(' ');
        }
    }
}

// internal itoa for 'long' type
void _ntoa_long(SprintfStrBuf& buf,
    unsigned long value, bool negative, int32_t base, int32_t prec, int32_t width, uint32_t flags)
{
    char tmpBuf[PRINTF_NTOA_BUFFER_SIZE];
    int32_t len = 0;

    // no hash for 0 values
    if (!value) {
        flags &= ~ParseFlag::Hash;
    }

    // write if precision != 0 and value is != 0
    if (!(flags & ParseFlag::Precision) || value) {
        do {
            const char digit = (char)(value % base);
            tmpBuf[len++] = digit < 10 ? '0' + digit : (flags & ParseFlag::Uppercase ? 'A' : 'a') + digit - 10;
            value /= base;
        } while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
    }

    _ntoa_format(buf, tmpBuf, len, negative, base, prec, width, flags);
}

// internal itoa for 'long long' type
void _ntoa_long_long(SprintfStrBuf& buf,
    unsigned long long value, bool negative, int32_t base, int32_t prec, int32_t width, uint32_t flags)
{
    char tmpBuf[PRINTF_NTOA_BUFFER_SIZE];
    int32_t len = 0;

    // no hash for 0 values
    if (!value) {
        flags &= ~ParseFlag::Hash;
    }

    // write if precision != 0 and value is != 0
    if (!(flags & ParseFlag::Precision) || value) {
        do {
            const char digit = (char)(value % base);
            tmpBuf[len++] = digit < 10 ? '0' + digit : (flags & ParseFlag::Uppercase ? 'A' : 'a') + digit - 10;
            value /= base;
        } while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
    }

    _ntoa_format(buf, tmpBuf, len, negative, base, prec, width, flags);
}

void _ftoa(SprintfStrBuf& buf, double value, int32_t prec, int32_t width, uint32_t flags)
{
    const int32_t start_idx = static_cast<int32_t>(buf.length());

    char tmpBuf[PRINTF_FTOA_BUFFER_SIZE];
    int32_t len = 0;
    double diff = 0.0;

    // if input is larger than thres_max, revert to exponential
    constexpr double thres_max = (double)0x7FFFFFFF;

    // powers of 10
    static constexpr double pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

    // test for NaN
    if (value != value) {
        buf.append("nan");
        return;
    }

    // test for negative
    bool negative = false;
    if (value < 0) {
        negative = true;
        value = 0 - value;
    }

    // set default precision to 6, if not set explicitly
    if (!(flags & ParseFlag::Precision)) {
        prec = 6;
    }
    // limit precision to 9, cause a prec >= 10 can lead to overflow errors
    while ((len < PRINTF_FTOA_BUFFER_SIZE) && (prec > 9)) {
        tmpBuf[len++] = '0';
        prec--;
    }

    int32_t whole = static_cast<int32_t>(value);
    double tmp = (value - whole) * pow10[prec];
    unsigned long frac = (unsigned long)tmp;
    diff = tmp - frac;

    if (diff > 0.5) {
        ++frac;
        // handle rollover, e.g. case 0.99 with prec 1 is 1.0
        if (frac >= pow10[prec]) {
            frac = 0;
            ++whole;
        }
    }
    else if (diff < 0.5) {
    }
    else if ((frac == 0) || (frac & 1)) {
        // if halfway, round up if odd OR if last digit is 0
        ++frac;
    }

    // TBD: for very large numbers switch back to native sprintf for exponentials. Anyone want to write code to replace this?
    // Normal printf behavior is to print EVERY whole number digit which can be 100s of characters overflowing your buffers == bad
#if 1
    if (value > thres_max) {
        return;
    }
#endif

    if (prec == 0) {
        diff = value - (double)whole;
        if ((!(diff < 0.5) || (diff > 0.5)) && (whole & 1)) {
            // exactly 0.5 and ODD, then round up
            // 1.5 -> 2, but 2.5 -> 2
            ++whole;
        }
    }
    else {
        int32_t count = prec;
        // now do fractional part, as an unsigned number
        while (len < PRINTF_FTOA_BUFFER_SIZE) {
            --count;
            tmpBuf[len++] = (char)(48 + (frac % 10));
            if (!(frac /= 10)) {
                break;
            }
        }
        // add extra 0s
        while ((len < PRINTF_FTOA_BUFFER_SIZE) && (count-- > 0)) {
            tmpBuf[len++] = '0';
        }
        if (len < PRINTF_FTOA_BUFFER_SIZE) {
            // add decimal
            tmpBuf[len++] = '.';
        }
    }

    // do whole part, number is reversed
    while (len < PRINTF_FTOA_BUFFER_SIZE) {
        tmpBuf[len++] = (char)(48 + (whole % 10));
        if (!(whole /= 10)) {
            break;
        }
    }

    // pad leading zeros
    if (!(flags & ParseFlag::Left) && (flags & ParseFlag::Zeropad)) {
        if (width && (negative || (flags & (ParseFlag::Plus | ParseFlag::Space)))) {
            width--;
        }
        while ((len < width) && (len < PRINTF_FTOA_BUFFER_SIZE)) {
            tmpBuf[len++] = '0';
        }
    }

    if (len < PRINTF_FTOA_BUFFER_SIZE) {
        if (negative) {
            tmpBuf[len++] = '-';
        }
        else if (flags & ParseFlag::Plus) {
            tmpBuf[len++] = '+'; // ignore the space if the '+' exists
        }
        else if (flags & ParseFlag::Space) {
            tmpBuf[len++] = ' ';
        }
    }

    // pad spaces up to given width
    if (!(flags & ParseFlag::Left) && !(flags & ParseFlag::Zeropad)) {
        for (int32_t i = len; i < width; i++) {
            buf.append(' ');
        }
    }

    // reverse string
    for (int32_t i = 0; i < len; i++) {
        buf.append(tmpBuf[len - i - 1]);
    }

    // append pad spaces up to given width
    if (flags & ParseFlag::Left) {
        while (static_cast<int32_t>(buf.length()) - start_idx < width) {
            buf.append(' ');
        }
    }
}


void _vsnprintf(SprintfStrBuf& buf, const char* pFormat, ArgDataHelper& va)
{
    uint32_t flags;
    int32_t width, precision;

    // %[flags][width][.precision][length]
    while (*pFormat) {
        if (*pFormat != '%') {
            buf.append(*pFormat);
            pFormat++;
            continue;
        }
        else {
            pFormat++;
        }

        // evaluate flags
        flags = 0U;
        bool loop = false;

        do {
            switch (*pFormat) {
                case '0':
                    flags |= ParseFlag::Zeropad;
                    loop = true;
                    break;
                case '-':
                    flags |= ParseFlag::Left;
                    loop = true;
                    break;
                case '+':
                    flags |= ParseFlag::Plus;
                    loop = true;
                    break;
                case ' ':
                    flags |= ParseFlag::Space;
                    loop = true;
                    break;
                case '#':
                    flags |= ParseFlag::Hash;
                    loop = true;
                    break;
                default:
                    loop = false;
                    break;
            }

            if (loop) {
                ++pFormat;
            }

        } while (loop);

        // evaluate width field
        width = 0;
        if (_is_digit(*pFormat)) {
            width = _atoi(&pFormat);
        }
        else if (*pFormat == '*') {
            const int32_t w = va.get<int>();
            if (w < 0) {
                flags |= ParseFlag::Left; // reverse padding
                width = -w;
            }
            else {
                width = w;
            }
            pFormat++;
        }

        // evaluate precision field
        precision = 0;
        if (*pFormat == '.') {
            flags |= ParseFlag::Precision;
            pFormat++;
            if (_is_digit(*pFormat)) {
                precision = _atoi(&pFormat);
            }
            else if (*pFormat == '*') {
                const int32_t prec = va.get<int>();
                precision = prec > 0 ? prec : 0;
                pFormat++;
            }
        }

        // evaluate length field
        switch (*pFormat) {
            case 'l':
                flags |= ParseFlag::Long;
                pFormat++;
                if (*pFormat == 'l') {
                    flags |= ParseFlag::LongLong;
                    pFormat++;
                }
                break;
            case 'h':
                flags |= ParseFlag::Short;
                pFormat++;
                if (*pFormat == 'h') {
                    flags |= ParseFlag::Char;
                    pFormat++;
                }
                break;
            case 't':
                flags |= (sizeof(ptrdiff_t) == sizeof(long) ? ParseFlag::Long : ParseFlag::LongLong);
                pFormat++;
                break;
            case 'j':
                flags |= (sizeof(intmax_t) == sizeof(long) ? ParseFlag::Long : ParseFlag::LongLong);
                pFormat++;
                break;
            case 'z':
                flags |= (sizeof(size_t) == sizeof(long) ? ParseFlag::Long : ParseFlag::LongLong);
                pFormat++;
                break;
            default:
                break;
        }

        // evaluate specifier
        switch (*pFormat) {
            case 'd':
            case 'i':
            case 'u':
            case 'x':
            case 'X':
            case 'o':
            case 'b': {
                // set the base
                int32_t base;
                if (*pFormat == 'x' || *pFormat == 'X') {
                    base = 16;
                }
                else if (*pFormat == 'o') {
                    base = 8;
                }
                else if (*pFormat == 'b') {
                    base = 2;
                }
                else {
                    base = 10;
                    flags &= ~ParseFlag::Hash; // no hash for dec pFormat
                }
                // uppercase
                if (*pFormat == 'X') {
                    flags |= ParseFlag::Uppercase;
                }

                // no plus or space flag for u, x, X, o, b
                if ((*pFormat != 'i') && (*pFormat != 'd')) {
                    flags &= ~(ParseFlag::Plus | ParseFlag::Space);
                }

                // ignore '0' flag when precision is given
                if (flags & ParseFlag::Precision) {
                    flags &= ~ParseFlag::Zeropad;
                }

                // convert the integer
                if ((*pFormat == 'i') || (*pFormat == 'd')) {
                    // signed
                    if (flags & ParseFlag::LongLong) {
                        const long long value = va.get<long long>();
                        _ntoa_long_long(buf, (unsigned long long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
                    }
                    else if (flags & ParseFlag::Long) {
                        const long value = va.get<long>();
                        _ntoa_long(buf, (unsigned long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
                    }
                    else {
                        const int value = (flags & ParseFlag::Char) ? (char)va.get<int>() : (flags & ParseFlag::Short) ? (short int)va.get<int>() : va.get<int>();
                        _ntoa_long(buf, (uint32_t)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
                    }
                }
                else {
                    // unsigned
                    if (flags & ParseFlag::LongLong) {
                        _ntoa_long_long(buf, va.get<unsigned long long>(), false, base, precision, width, flags);
                    }
                    else if (flags & ParseFlag::Long) {
                        _ntoa_long(buf, va.get<unsigned long>(), false, base, precision, width, flags);
                    }
                    else {
                        const uint32_t value = (flags & ParseFlag::Char) ? (unsigned char)va.get<unsigned int>() : (flags & ParseFlag::Short) ? (unsigned short int)va.get<unsigned int>() : va.get<unsigned int>();
                        _ntoa_long(buf, value, false, base, precision, width, flags);
                    }
                }
                pFormat++;
                break;
            }

            case 'f':
            case 'F':
                _ftoa(buf, va.get<double>(), precision, width, flags);
                pFormat++;
                break;

            case 'c': {
                int32_t l = 1;
                // pre padding
                if (!(flags & ParseFlag::Left)) {
                    while (l++ < width) {
                        buf.append(' ');
                    }
                }
                // char output
                buf.append((char)va.get<int>());

                // post padding
                if (flags & ParseFlag::Left) {
                    while (l++ < width) {
                        buf.append(' ');
                    }
                }
                pFormat++;
                break;
            }

            case 's': {
                auto str = va.getString();

                const char* pStr = str.first;
                int32_t l = str.second;

                // pre padding
                if (flags & ParseFlag::Precision) {
                    l = (l < precision ? l : precision);
                }
                else {
                    precision = l;
                }

                if (!(flags & ParseFlag::Left)) {
                    while (l++ < width) {
                        buf.append(' ');
                    }
                }
                // string output
                while (precision--) {
                    buf.append(*(pStr++));
                }
                // post padding
                if (flags & ParseFlag::Left) {
                    while (l++ < width) {
                        buf.append(' ');
                    }
                }
                pFormat++;
                break;
            }

            case 'p': {
                width = sizeof(void*) * 2;
                flags |= ParseFlag::Zeropad | ParseFlag::Uppercase;
                const bool is_ll = sizeof(uintptr_t) == sizeof(long long);
                if (is_ll) {
                    _ntoa_long_long(buf, (uintptr_t)va.get<void*>(), false, 16, precision, width, flags);
                }
                else {
                    _ntoa_long(buf, (unsigned long)((uintptr_t)va.get<void*>()), false, 16, precision, width, flags);
                }
                pFormat++;
                break;
            }

            case '%':
                buf.append('%');
                pFormat++;
                break;

            default:
                buf.append(*pFormat);
                pFormat++;
                break;
        }
    }

}


void sprintf_ArgData(SprintfStrBuf& buf, const char* pFormat, const ArgData& data)
{
    ArgDataHelper vaShim(data);
    _vsnprintf(buf, pFormat, vaShim);
}

X_NAMESPACE_END
