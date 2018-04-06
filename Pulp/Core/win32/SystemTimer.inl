

namespace SysTimer
{
    X_INLINE int64_t Get(void)
    {
        extern TimeUpdateFunc::Pointer g_pUpdateFunc;

        return g_pUpdateFunc();
    }

    X_INLINE float ToSeconds(int64_t count)
    {
        extern float g_oneOverFrequency;
        return count * g_oneOverFrequency;
    }

    X_INLINE float ToMilliSeconds(int64_t count)
    {
        extern float g_thousandOverFrequency;
        return count * g_thousandOverFrequency;
    }

    X_INLINE int64_t fromSeconds(float value)
    {
        // times by frequency.
        extern float g_FrequencySingle;
        return static_cast<int64_t>(value * g_FrequencySingle);
    }
    X_INLINE int64_t fromSeconds(double value)
    {
        extern double g_FrequencyDouble;
        return static_cast<int64_t>(value * g_FrequencyDouble);
    }

    X_INLINE int64_t fromSeconds(int64_t value)
    {
        return value * GetTickPerSec();
    }

    X_INLINE int64_t fromMilliSeconds(int32_t value)
    {
        return fromMilliSeconds(static_cast<int64_t>(value));
    }

    X_INLINE int64_t fromMilliSeconds(float value)
    {
        extern float g_MilliToValueSingle;
        return static_cast<int64_t>(value * g_MilliToValueSingle);
    }

    X_INLINE int64_t fromMilliSeconds(double value)
    {
        extern double g_MilliToValueDouble;
        return static_cast<int64_t>(value * g_MilliToValueDouble);
    }

    X_INLINE int64_t fromMilliSeconds(int64_t value)
    {
        extern double g_MilliToValueDouble;
        return static_cast<int64_t>(value * g_MilliToValueDouble);
    }

    X_INLINE int64_t fromMicroSeconds(int64_t value)
    {
        extern double g_MicroToValueDouble;
        return static_cast<int64_t>(value * g_MicroToValueDouble);
    }

    X_INLINE int64_t fromNanoSeconds(int64_t value)
    {
        extern double g_NanoToValueDouble;
        return static_cast<int64_t>(value * g_NanoToValueDouble);
    }

    X_INLINE int64 GetTickPerSec()
    {
        extern int64_t g_Frequency;
        return g_Frequency;
    }

    //
    //	how to share these values across the engine.
    //
    //
    //
    //
} // namespace SysTimer
