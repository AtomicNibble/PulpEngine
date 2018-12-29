#pragma once

X_NAMESPACE_BEGIN(core)

// This is a type safe string format helper.
// It's used for format spcifiers loaded from disk.
// instead of '%' '{}' must be used.
// Example:
//
//    core::StackString256 str;
//    core::format::format_to(str, "{},{},{},{},{} -> {{", 42, std::string_view("goat"), 0.5, 1.f, 51515_ui64);
//

namespace format
{
    namespace Internal
    {
        // Prefixed with t_ to avoid name clashes
        X_DECLARE_ENUM(ValueType)
        (
            none,
            // Integer types
            t_int,
            t_uint,
            t_long_long,
            t_ulong_long,
            t_bool,
            t_char,

            // floating-point types.
            t_double,

            t_cstring,
            t_string,
            t_pointer);

        constexpr bool isIntegral(ValueType::Enum t)
        {
            return t <= ValueType::t_char;
        }

        constexpr bool isArithmetic(ValueType::Enum t)
        {
            return t <= ValueType::t_double;
        }

        struct string_value
        {
            const char* pValue;
            std::size_t size;
        };

        // A formatting argument value.
        class value
        {
        public:
            constexpr value(int val = 0) :
                int_value(val)
            {
            }
            value(unsigned val) :
                uint_value(val)
            {
            }
            value(long long val) :
                long_long_value(val)
            {
            }
            value(unsigned long long val) :
                ulong_long_value(val)
            {
            }
            value(double val) :
                double_value(val)
            {
            }
            value(const void* val) :
                pPointer(val)
            {
            }
            value(const char* val)
            {
                string.pValue = val;
            }
            value(std::basic_string_view<char> val)
            {
                string.pValue = val.data();
                string.size = val.size();
            }

            union
            {
                int int_value;
                unsigned uint_value;
                long long long_long_value;
                unsigned long long ulong_long_value;
                double double_value;
                const void* pPointer;
                string_value string;
            };
        };

        template<typename T, typename Char, typename Enable = void>
        struct convert_to_int
        {
            enum
            {
                value = !std::is_arithmetic<T>::value && std::is_convertible<T, int>::value
            };
        };

        template<typename T, ValueType::Enum TYPE>
        struct init
        {
            static const ValueType::Enum type_tag = TYPE;

            constexpr init(const T& v) :
                val(v)
            {
            }

            constexpr operator value() const
            {
                return value(val);
            }

            T val;
        };

        template<typename T>
        typename std::enable_if<!std::is_same<T, char>::value>::type make_value(const T*)
        {
            static_assert(!sizeof(T), "formatting of non-void pointers is disallowed");
        }

        template<typename T>
        inline typename std::enable_if<
            std::is_enum<T>::value && convert_to_int<T, char>::value,
            init<int, ValueType::t_int>>::type
            make_value(const T& val)
        {
            return static_cast<int>(val);
        }

        template<typename T>
        inline typename std::enable_if<
            std::is_constructible<std::basic_string_view<char>, T>::value,
            init<std::basic_string_view<char>, ValueType::t_string>>::type
            make_value(const T& val)
        {
            return std::basic_string_view<char>(val);
        }

#define MAKE_VALUE(TAG, ArgType, ValueType)                \
    constexpr init<ValueType, TAG> make_value(ArgType val) \
    {                                                      \
        return static_cast<ValueType>(val);                \
    }

#define MAKE_VALUE_SAME(TAG, Type)                 \
    constexpr init<Type, TAG> make_value(Type val) \
    {                                              \
        return val;                                \
    }

        MAKE_VALUE(ValueType::t_bool, bool, int)
        MAKE_VALUE(ValueType::t_int, short, int)
        MAKE_VALUE(ValueType::t_uint, unsigned short, unsigned)
        MAKE_VALUE_SAME(ValueType::t_int, int)
        MAKE_VALUE_SAME(ValueType::t_uint, unsigned)
        MAKE_VALUE_SAME(ValueType::t_long_long, long long)
        MAKE_VALUE_SAME(ValueType::t_ulong_long, unsigned long long)
        MAKE_VALUE(ValueType::t_int, signed char, int)
        MAKE_VALUE(ValueType::t_uint, unsigned char, unsigned)
        MAKE_VALUE(ValueType::t_char, char, int)

        typedef std::conditional<sizeof(long) == sizeof(int), int, long long>::type long_type;
        typedef std::conditional<sizeof(unsigned long) == sizeof(unsigned), unsigned, unsigned long long>::type ulong_type;

        MAKE_VALUE((sizeof(long) == sizeof(int) ? ValueType::t_int : ValueType::t_long_long), long, long_type)
        MAKE_VALUE((sizeof(unsigned long) == sizeof(unsigned) ? ValueType::t_uint : ValueType::t_ulong_long), unsigned long, ulong_type)

        MAKE_VALUE(ValueType::t_double, float, double)
        MAKE_VALUE_SAME(ValueType::t_double, double)

        // const strings
        MAKE_VALUE(ValueType::t_cstring, typename char*, const typename char*)
        MAKE_VALUE(ValueType::t_cstring, const typename char*, const typename char*)
        MAKE_VALUE(ValueType::t_cstring, signed char*, const signed char*)
        MAKE_VALUE_SAME(ValueType::t_cstring, const signed char*)
        MAKE_VALUE(ValueType::t_cstring, unsigned char*, const unsigned char*)
        MAKE_VALUE_SAME(ValueType::t_cstring, const unsigned char*)

        // views and strings
        MAKE_VALUE_SAME(ValueType::t_string, std::basic_string_view<char>)
        MAKE_VALUE(ValueType::t_string, const std::basic_string<char>&, std::basic_string_view<char>)

        MAKE_VALUE(ValueType::t_pointer, void*, const void*)
        MAKE_VALUE_SAME(ValueType::t_pointer, const void*)
        MAKE_VALUE(ValueType::t_pointer, std::nullptr_t, const void*)

        template<typename T>
        inline value make_arg(const T& value)
        {
            return make_value(value);
        }

        template<typename T>
        struct get_type
        {
            typedef decltype(make_value(std::declval<typename std::decay<T>::type&>())) value_type;

            static const ValueType::Enum value = value_type::type_tag;
        };

        static constexpr int BITS_PER_TYPE = 4;

        static_assert(ValueType::ENUM_COUNT < (1 << BITS_PER_TYPE), "Can't store all types in 4 bis");

        template<typename Last>
        constexpr uint64_t get_types()
        {
            return get_type<Last>::value;
        }

        template<typename First, typename Second, typename... Args>
        constexpr uint64_t get_types()
        {
            return get_type<First>::value | (get_types<Second, Args...>() << BITS_PER_TYPE);
        }

        template<typename... Args>
        class format_arg_store
        {
            static const size_t NUM_ARGS = sizeof...(Args);
            static const size_t DATA_SIZE = NUM_ARGS;

            static_assert((NUM_ARGS * BITS_PER_TYPE) < sizeof(uint64_t) * 8, "Too many args");

            typedef value value_type;

            friend class basic_format_args;

        public:
            format_arg_store(const Args&... args) :
                data_{make_arg(args)...}
            {
            }

            static constexpr uint64_t get_types_int()
            {
                return get_types<Args...>();
            }

            static constexpr uint64_t TYPES = get_types_int();

        private:
            value_type data_[DATA_SIZE];
        };

        template<typename... Args>
        inline format_arg_store<Args...> make_format_args(const Args&... args)
        {
            return format_arg_store<Args...>(args...);
        }

        class basic_format_arg
        {
            friend class basic_format_args;

        public:
            constexpr basic_format_arg() :
                type_(ValueType::none)
            {
            }

            explicit operator bool() const
            {
                return type_ != ValueType::none;
            }

            ValueType::Enum type() const
            {
                return type_;
            }

            const value& getValue(void) const
            {
                return value_;
            }

            bool isIntegral(void) const
            {
                return Internal::isIntegral(type_);
            }
            bool isArithmetic(void) const
            {
                return Internal::isArithmetic(type_);
            }

        private:
            value value_;
            ValueType::Enum type_;
        };

        class basic_format_args
        {
            typedef basic_format_arg format_arg;
            typedef int32_t index_type;

        public:
            basic_format_args() :
                types_(0)
            {
            }

            template<typename... Args>
            basic_format_args(const format_arg_store<Args...>& store) :
                types_(store.TYPES),
                pValues_(store.data_)
            {
            }

            format_arg get(index_type index) const
            {
                format_arg arg;
                arg.type_ = type(index);
                if (arg.type() == ValueType::none) {
                    return arg;
                }

                arg.value_ = pValues_[index];
                return arg;
            }

        private:
            typename ValueType::Enum type(index_type index) const
            {
                uint32_t shift = index * BITS_PER_TYPE;
                uint64_t mask = 0xf;
                return static_cast<ValueType::Enum>((types_ & (mask << shift)) >> shift);
            }

        private:
            uint64_t types_;
            const value* pValues_;
        };

        struct format_args : basic_format_args
        {
            template<typename... Args>
            format_args(Args&&... arg) :
                basic_format_args(std::forward<Args>(arg)...)
            {
            }
        };

        template<size_t N, typename... Args>
        void format_to_args(core::StackString<N>& buf, core::string_view fmt, format_args args)
        {
            buf.clear();

            // don't use the string view iterator it's balls to the wall full of debug checks.
            const char* pBegin = fmt.data();
            const char* pEnd = fmt.data() + fmt.length();

            int32_t idx = 0;

            while (pBegin != pEnd) {
                // any format shit?
                const char* p = pBegin;

                if (*pBegin != '{') {
                    p = core::strUtil::Find(pBegin, pEnd, '{');
                    if (!p) {
                        // write it all.
                        buf.append(pBegin, pEnd);
                        return;
                    }

                    // write begin - p
                    buf.append(pBegin, p);
                }

                ++p;

                if (p == pEnd) {
                    // missing closing '}'
                    return;
                }

                if (*p == '}') {
                    // gimmy them args boi.
                    auto arg = args.get(idx++);
                    auto& value = arg.getValue();

                    switch (arg.type()) {
                        case ValueType::none:
                            // we ran out of args?
                            break;

                        case ValueType::t_int:
                            buf.appendFmt("%" PRIi32, value.int_value);
                            break;
                        case ValueType::t_uint:
                            buf.appendFmt("%" PRIu32, value.int_value);
                            break;
                        case ValueType::t_long_long:
                            buf.appendFmt("%" PRIu64, value.long_long_value);
                            break;
                        case ValueType::t_ulong_long:
                            buf.appendFmt("%" PRIu64, value.ulong_long_value);
                            break;
                        case ValueType::t_bool:
                            if (value.int_value) {
                                buf.append("true");
                            }
                            else {
                                buf.append("false");
                            }
                            break;
                        case ValueType::t_char:
                            buf.append(static_cast<char>(value.int_value), 1);
                            break;
                        case ValueType::t_double:
                            buf.appendFmt("%g", value.double_value);
                            break;

                        case ValueType::t_cstring:
                            buf.append(value.string.pValue);
                            break;
                        case ValueType::t_string:
                            buf.append(value.string.pValue, value.string.size);
                            break;
                        case ValueType::t_pointer:
                            buf.appendFmt("%p", value.pPointer);
                            break;

                        default:
                            X_ASSERT_UNREACHABLE();
                            break;
                    }
                }
                else if (*p == '{') { // escape {
                    buf.append('{', 1);
                }
                else {
                    X_ASSERT_NOT_IMPLEMENTED();
                    return;
                }

                pBegin = p + 1;
            }
        }

    } // namespace Internal

    template<size_t N, typename... Args>
    void format_to(core::StackString<N>& buf, core::string_view fmt, Args... args)
    {
        Internal::format_to_args(buf, fmt, Internal::make_format_args(args...));
    }

} // namespace format

X_NAMESPACE_END