

template<typename TO, typename FROM>
inline TO union_cast(FROM from)
{
    static_assert(sizeof(TO) == sizeof(FROM), "Size mismatch. Cannot use a union_cast for types of different sizes.");

    union
    {
        FROM castFrom;
        TO castTo;
    };

    castFrom = from;
    return castTo;
}
