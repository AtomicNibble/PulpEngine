
X_NAMESPACE_BEGIN(core)

namespace internal
{
    /// Template specialization for casting from an unsigned type into an unsigned type
    template<>
    struct safe_static_cast_helper<false, false>
    {
        template<typename TO, typename FROM>
        static inline TO cast(FROM from)
        {
            X_ASSERT(from >= std::numeric_limits<TO>::min(), "Number to cast exceeds numeric limits.")(from, TO(), std::numeric_limits<TO>::min(), std::numeric_limits<TO>::max());
            X_ASSERT(from <= std::numeric_limits<TO>::max(), "Number to cast exceeds numeric limits.")(from, TO(), std::numeric_limits<TO>::min(), std::numeric_limits<TO>::max());
            return static_cast<TO>(from);
        }
    };

    /// Template specialization for casting from an unsigned type into a signed type
    template<>
    struct safe_static_cast_helper<false, true>
    {
        template<typename TO, typename FROM>
        static inline TO cast(FROM from)
        {
            X_ASSERT(from <= static_cast<typename X_NAMESPACE(core)::traits::UnsignedType<TO>::Type>(std::numeric_limits<TO>::max()), "Number to cast exceeds numeric limits.")(from, TO(), std::numeric_limits<TO>::min(), std::numeric_limits<TO>::max());
            X_ASSERT(static_cast<TO>(from) >= 0, "Number to cast exceeds numeric limits.")(from, TO(), std::numeric_limits<TO>::min(), std::numeric_limits<TO>::max());
            return static_cast<TO>(from);
        }
    };

    /// Template specialization for casting from a signed type into an unsigned type
    template<>
    struct safe_static_cast_helper<true, false>
    {
        template<typename TO, typename FROM>
        static inline TO cast(FROM from)
        {
            // make sure the input is not negative
            X_ASSERT(from >= 0, "Number to cast exceeds numeric limits.")(from, TO(), std::numeric_limits<TO>::min(), std::numeric_limits<TO>::max());

            // assuring a positive input, we can safely cast it into its unsigned type and check the numeric limits
            X_ASSERT(static_cast<typename X_NAMESPACE(core)::traits::UnsignedType<FROM>::Type>(from) >= std::numeric_limits<TO>::min(), "Number to cast exceeds numeric limits.")(from, TO(), std::numeric_limits<TO>::min(), std::numeric_limits<TO>::max());
            X_ASSERT(static_cast<typename X_NAMESPACE(core)::traits::UnsignedType<FROM>::Type>(from) <= std::numeric_limits<TO>::max(), "Number to cast exceeds numeric limits.")(from, TO(), std::numeric_limits<TO>::min(), std::numeric_limits<TO>::max());

            return static_cast<TO>(from);
        }
    };

    /// Template specialization for casting from a signed type into a signed type
    template<>
    struct safe_static_cast_helper<true, true>
    {
        template<typename TO, typename FROM>
        static inline TO cast(FROM from)
        {
            X_ASSERT(from >= std::numeric_limits<TO>::min(), "Number to cast exceeds numeric limits.")(from, TO(), std::numeric_limits<TO>::min(), std::numeric_limits<TO>::max());
            X_ASSERT(from <= std::numeric_limits<TO>::max(), "Number to cast exceeds numeric limits.")(from, TO(), std::numeric_limits<TO>::min(), std::numeric_limits<TO>::max());
            return static_cast<TO>(from);
        }
    };
} // namespace internal

X_NAMESPACE_END
