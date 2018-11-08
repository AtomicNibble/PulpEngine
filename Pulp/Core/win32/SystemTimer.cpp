#include "stdafx.h"
#include "SystemTimer.h"

#include <Util\LastError.h>

#ifndef NEAR
#define NEAR near
#endif

#ifndef FAR
#define FAR far
#endif

#include "Mmsystem.h"

// needed for timeGetTime
// Delay loaded now.
// X_LINK_LIB("Winmm.lib")

X_NAMESPACE_BEGIN(core)

namespace SysTimer
{
    TimeUpdateFunc::Pointer g_pUpdateFunc;

    float g_oneOverFrequency;
    float g_thousandOverFrequency;
    float g_FrequencySingle;
    double g_FrequencyDouble;

    float g_MilliToValueSingle;
    double g_MilliToValueDouble;
    double g_MicroToValueDouble;
    double g_NanoToValueDouble;

    int64_t g_Frequency;

    namespace
    {
        int64_t PerformanceCounterTime(void)
        {
            LARGE_INTEGER now = {};
            QueryPerformanceCounter(&now);
            return static_cast<int64_t>(now.QuadPart);
        }

        int64_t MMTimeGet(void)
        {
            return static_cast<int64_t>(timeGetTime());
        }

        void UpdateHelpers(void)
        {
            double resolution = 1.0 / static_cast<double>(g_Frequency);

            g_FrequencySingle = static_cast<float>(g_Frequency);
            g_FrequencyDouble = static_cast<double>(g_Frequency);
            // times it by frequency * 1000
            g_MilliToValueSingle = static_cast<float>(g_FrequencyDouble / 1000);
            g_MilliToValueDouble = static_cast<double>(g_FrequencyDouble / 1000);
            g_MicroToValueDouble = static_cast<double>(g_FrequencyDouble / (1000 * 1000));
            g_NanoToValueDouble = static_cast<double>(g_FrequencyDouble / (1000 * 1000 * 1000));

            g_oneOverFrequency = static_cast<float>(resolution);
            g_thousandOverFrequency = static_cast<float>(resolution * 1000.0);
        }

    } // namespace

    void Startup(void)
    {
        LARGE_INTEGER frequency;

        if (QueryPerformanceFrequency(&frequency)) {
            g_Frequency = frequency.QuadPart;
            g_pUpdateFunc = &PerformanceCounterTime;
        }
        else {
            lastError::Description Dsc;
            X_WARNING("SysTimer", "Failed to query performance freq. Error: ", lastError::ToString(Dsc));

            g_Frequency = 1000;
            g_pUpdateFunc = &MMTimeGet;
        }

        UpdateHelpers();
    }

    void Shutdown(void)
    {
        g_oneOverFrequency = 0;
        g_thousandOverFrequency = 0;
    }

    bool HasFreqChanged(void)
    {
        if (g_pUpdateFunc != &PerformanceCounterTime) {
            return false;
        }

        LARGE_INTEGER frequency;

        if (!QueryPerformanceFrequency(&frequency)) {
            lastError::Description Dsc;
            X_WARNING("SysTimer", "Failed to query performance freq. Error: ", lastError::ToString(Dsc));
            return false;
        }

        if (frequency.QuadPart == g_Frequency) {
            return false;
        }

        X_WARNING("SysTime", "Freq changed from %" PRIu64 " tp %" PRIu64, g_Frequency, frequency.QuadPart);

        g_Frequency = frequency.QuadPart;

        UpdateHelpers();
        return true;
    }

} // namespace SysTimer

X_NAMESPACE_END