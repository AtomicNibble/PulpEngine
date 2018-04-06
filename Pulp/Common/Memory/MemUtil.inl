
X_NAMESPACE_BEGIN(core)

namespace Mem
{
    namespace Internal
    {
        // these ripped out std headers, just cus they are not exposed so they may change or be removed.

        struct _General_ptr_iterator_tag
        { // general case, no special optimizations
        };

        struct _Trivially_copyable_ptr_iterator_tag
            : _General_ptr_iterator_tag
        { // iterator is a pointer to a trivially copyable type
        };

        struct _Really_trivial_ptr_iterator_tag
            : _Trivially_copyable_ptr_iterator_tag
        { // iterator is a pointer to a trivial type
        };

        template<class _Ty1, class _Ty2>
        struct _Is_same_size : std::bool_constant<sizeof(_Ty1) == sizeof(_Ty2)>
        { // determine whether two types have the same size
        };

        // STRUCT TEMPLATE _Unwrap_enum
        template<class _Elem, bool _Is_enum = std::is_enum<_Elem>::value>
        struct _Unwrap_enum
        { // if _Elem is an enum, gets its underlying type; otherwise leaves _Elem unchanged
            typedef std::underlying_type_t<_Elem> type;
        };

        template<class _Elem>
        struct _Unwrap_enum<_Elem, false>
        { // passthrough non-enum type
            typedef _Elem type;
        };

        // STRUCT TEMPLATE _Both_or_neither_bool
        template<class _Ty1, class _Ty2>
        struct _Both_or_neither_bool : std::bool_constant<std::is_same<bool, _Ty1>::value == std::is_same<bool, _Ty2>::value>
        { // determines if both _Ty1 and _Ty2 are exactly bool, or neither are
        };

        template<class _Source, class _Dest>
        struct _Ptr_cat_helper
        {
            // determines _Ptr_cat's result in the most general case
            typedef typename _Unwrap_enum<_Source>::type _USource;
            typedef typename _Unwrap_enum<_Dest>::type _UDest;
            typedef std::conditional_t<
                std::conjunction<
                    _Is_same_size<_USource, _UDest>,
                    std::is_integral<_USource>,
                    std::is_integral<_UDest>,
                    _Both_or_neither_bool<_USource, _UDest>,
                    // note: _Unwrap_enum strips volatile so we must check the type directly
                    std::negation<std::is_volatile<_Source>>,
                    std::negation<std::is_volatile<_Dest>>>::value,
                _Really_trivial_ptr_iterator_tag,
                _General_ptr_iterator_tag>
                type;
        };

        template<class _Elem>
        struct _Ptr_cat_helper<_Elem, _Elem>
        { // determines _Ptr_cat's result when the types are the same
            typedef std::conditional_t<
                std::is_trivially_copyable<_Elem>::value,
                std::conditional_t<std::is_trivial<_Elem>::value,
                    _Really_trivial_ptr_iterator_tag,
                    _Trivially_copyable_ptr_iterator_tag>,
                _General_ptr_iterator_tag>
                type;
        };

        template<class _Anything>
        struct _Ptr_cat_helper<_Anything*, const _Anything*>
        { // determines _Ptr_cat's result when all we do is add const to a pointer
            typedef _Really_trivial_ptr_iterator_tag type;
        };

        template<class _Source, class _Dest>
        inline _General_ptr_iterator_tag _Ptr_move_cat(const _Source&, const _Dest&)
        { // return pointer move optimization category for arbitrary iterators
            return {};
        }

        template<class _Source, class _Dest>
        inline std::conditional_t<std::is_trivially_assignable<_Dest&, _Source>::value,
            typename _Ptr_cat_helper<std::remove_const_t<_Source>, _Dest>::type,
            _General_ptr_iterator_tag>
            _Ptr_move_cat(_Source* const&, _Dest* const&)
        { // return pointer move optimization category for pointers
            return {};
        }

        template<class _InIt, class _OutIt>
        inline _OutIt Move(_InIt first, _InIt last, _OutIt dest, _General_ptr_iterator_tag)
        {
            for (; first != last; ++dest, (void)++first) {
                *dest = std::move(*first);
            }

            return dest;
        }

        template<class _InIt, class _OutIt>
        inline _OutIt Move(_InIt first, _InIt last, _OutIt dest, _Trivially_copyable_ptr_iterator_tag)
        {
            const char* const _First_ch = reinterpret_cast<const char*>(first);
            const char* const _Last_ch = reinterpret_cast<const char*>(last);
            char* const _Dest_ch = reinterpret_cast<char*>(dest);

            const size_t count = _Last_ch - _First_ch;
            std::memmove(_Dest_ch, _First_ch, count);
            return (reinterpret_cast<_OutIt>(_Dest_ch + count));
        }

    } // namespace Internal

    template<class _InIt, class _OutIt>
    inline _OutIt Move(_InIt first, _InIt last, _OutIt dest)
    {
        return Internal::Move(first, last, dest, Internal::_Ptr_move_cat(first, dest));
    }

} // namespace Mem

X_NAMESPACE_END