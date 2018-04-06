#pragma once

// X_NAMESPACE_BEGIN(core)

namespace TL
{
    class NullType
    {
    };

    template<class T, class U>
    struct Typelist
    {
        typedef T Head;
        typedef U Tail;
    };

    template<
        typename T1 = NullType, typename T2 = NullType, typename T3 = NullType,
        typename T4 = NullType, typename T5 = NullType, typename T6 = NullType,
        typename T7 = NullType, typename T8 = NullType, typename T9 = NullType,
        typename T10 = NullType, typename T11 = NullType, typename T12 = NullType,
        typename T13 = NullType, typename T14 = NullType, typename T15 = NullType,
        typename T16 = NullType, typename T17 = NullType, typename T18 = NullType>
    struct MakeTypelist
    {
    private:
        typedef typename MakeTypelist<
            T2, T3, T4,
            T5, T6, T7,
            T8, T9, T10,
            T11, T12, T13,
            T14, T15, T16,
            T17, T18>::Result TailResult;

    public:
        typedef Typelist<T1, TailResult> Result;
    };

    template<>
    struct MakeTypelist<>
    {
        typedef NullType Result;
    };

    // Length
    template<class TList>
    struct Length;
    template<>
    struct Length<NullType>
    {
        enum
        {
            value = 0
        };
    };

    template<class T, class U>
    struct Length<Typelist<T, U>>
    {
        enum
        {
            value = 1 + Length<U>::value
        };
    };

    // TypeAt
    template<class TList, unsigned int index>
    struct TypeAt;

    template<class Head, class Tail>
    struct TypeAt<Typelist<Head, Tail>, 0>
    {
        typedef Head Result;
    };

    template<class Head, class Tail, unsigned int i>
    struct TypeAt<Typelist<Head, Tail>, i>
    {
        typedef typename TypeAt<Tail, i - 1>::Result Result;
    };

    // TypeAtNonStrict
    template<class TList, unsigned int index,
        typename DefaultType = NullType>
    struct TypeAtNonStrict
    {
        typedef DefaultType Result;
    };

    template<class Head, class Tail, typename DefaultType>
    struct TypeAtNonStrict<Typelist<Head, Tail>, 0, DefaultType>
    {
        typedef Head Result;
    };

    template<class Head, class Tail, unsigned int i, typename DefaultType>
    struct TypeAtNonStrict<Typelist<Head, Tail>, i, DefaultType>
    {
        typedef typename TypeAtNonStrict<Tail, i - 1, DefaultType>::Result Result;
    };

    // IndexOf
    template<class TList, class T>
    struct IndexOf;

    template<class T>
    struct IndexOf<NullType, T>
    {
        enum
        {
            value = -1
        };
    };

    template<class T, class Tail>
    struct IndexOf<Typelist<T, Tail>, T>
    {
        enum
        {
            value = 0
        };
    };

    template<class Head, class Tail, class T>
    struct IndexOf<Typelist<Head, Tail>, T>
    {
    private:
        enum
        {
            temp = IndexOf<Tail, T>::value
        };

    public:
        enum
        {
            value = (temp == -1 ? -1 : 1 + temp)
        };
    };

    // Append
    template<class TList, class T>
    struct Append;

    template<>
    struct Append<NullType, NullType>
    {
        typedef NullType Result;
    };

    template<class T>
    struct Append<NullType, T>
    {
        typedef Typelist<T, NullType> Result;
    };

    template<class Head, class Tail>
    struct Append<NullType, Typelist<Head, Tail>>
    {
        typedef Typelist<Head, Tail> Result;
    };

    template<class Head, class Tail, class T>
    struct Append<Typelist<Head, Tail>, T>
    {
        typedef Typelist<Head,
            typename Append<Tail, T>::Result>
            Result;
    };

    // Erase
    template<class TList, class T>
    struct Erase;

    template<class T> // Specialization 1
    struct Erase<NullType, T>
    {
        typedef NullType Result;
    };

    template<class T, class Tail> // Specialization 2
    struct Erase<Typelist<T, Tail>, T>
    {
        typedef Tail Result;
    };

    template<class Head, class Tail, class T> // Specialization 3
    struct Erase<Typelist<Head, Tail>, T>
    {
        typedef Typelist<Head,
            typename Erase<Tail, T>::Result>
            Result;
    };

    // EraseAll
    template<class TList, class T>
    struct EraseAll;
    template<class T>
    struct EraseAll<NullType, T>
    {
        typedef NullType Result;
    };
    template<class T, class Tail>
    struct EraseAll<Typelist<T, Tail>, T>
    {
        // Go all the way down the list removing the type
        typedef typename EraseAll<Tail, T>::Result Result;
    };
    template<class Head, class Tail, class T>
    struct EraseAll<Typelist<Head, Tail>, T>
    {
        // Go all the way down the list removing the type
        typedef Typelist<Head,
            typename EraseAll<Tail, T>::Result>
            Result;
    };

    // NoDuplicates
    template<class TList>
    struct NoDuplicates;

    template<>
    struct NoDuplicates<NullType>
    {
        typedef NullType Result;
    };

    template<class Head, class Tail>
    struct NoDuplicates<Typelist<Head, Tail>>
    {
    private:
        typedef typename NoDuplicates<Tail>::Result L1;
        typedef typename Erase<L1, Head>::Result L2;

    public:
        typedef Typelist<Head, L2> Result;
    };

    // Replace
    template<class TList, class T, class U>
    struct Replace;

    template<class T, class U>
    struct Replace<NullType, T, U>
    {
        typedef NullType Result;
    };

    template<class T, class Tail, class U>
    struct Replace<Typelist<T, Tail>, T, U>
    {
        typedef Typelist<U, Tail> Result;
    };

    template<class Head, class Tail, class T, class U>
    struct Replace<Typelist<Head, Tail>, T, U>
    {
        typedef Typelist<Head,
            typename Replace<Tail, T, U>::Result>
            Result;
    };

    // ReplaceAll
    template<class TList, class T, class U>
    struct ReplaceAll;

    template<class T, class U>
    struct ReplaceAll<NullType, T, U>
    {
        typedef NullType Result;
    };

    template<class T, class Tail, class U>
    struct ReplaceAll<Typelist<T, Tail>, T, U>
    {
        typedef Typelist<U, typename ReplaceAll<Tail, T, U>::Result> Result;
    };

    template<class Head, class Tail, class T, class U>
    struct ReplaceAll<Typelist<Head, Tail>, T, U>
    {
        typedef Typelist<Head,
            typename ReplaceAll<Tail, T, U>::Result>
            Result;
    };

    // Reverse
    template<class TList>
    struct Reverse;

    template<>
    struct Reverse<NullType>
    {
        typedef NullType Result;
    };

    template<class Head, class Tail>
    struct Reverse<Typelist<Head, Tail>>
    {
        typedef typename Append<
            typename Reverse<Tail>::Result, Head>::Result Result;
    };

} // namespace TL

// X_NAMESPACE_END