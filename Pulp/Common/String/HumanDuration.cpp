#include "EngineCommon.h"
#include "HumanDuration.h"

X_NAMESPACE_BEGIN(core)

namespace HumanDuration
{
    const char* toString(Str& str, float ms)
    {
        str.clear();

        if (ms <= 1000.f) {
            str.appendFmt("%.3f ms", ms);
        }
        else if (ms <= 300000.f) {
            const float sec = (ms / 1000.f);
            str.appendFmt("%.2f sec", sec);
        }
        else if (ms <= 16777216.f) {
            const float sec = (ms / 1000.f);
            const float min = (sec / 60.f);

            str.appendFmt("%.2f min", min);
        }
        else {
            const float sec = (ms / 1000.f);
            const float min = (sec / 60.f);

            str.appendFmt("%.2f min", min);
        }

        return str.c_str();
    }

    const char* toString(Str& str, int64_t ms)
    {
        str.clear();

        if (ms <= 1000_i64) {
            str.appendFmt("%" PRIi64 " ms", ms);
        }
        else if (ms <= 300000_i64) {
            const float sec = static_cast<float>(ms) / 1000.f;
            str.appendFmt("%.2f sec", sec);
        }
        else if (ms <= 16777216_i64) {
            const float sec = static_cast<float>(ms) / 1000.f;
            const float min = (sec / 60.f);

            str.appendFmt("%.2f min", min);
        }
        else {
            const float sec = static_cast<float>(ms) / 1000.f;
            const float min = (sec / 60.f);

            str.appendFmt("%.2f min", min);
        }

        return str.c_str();
    }

    const char* toStringMicro(Str& str, int64_t micro)
    {
        str.clear();

        if (micro <= 1000_i64) {
            str.setFmt("%" PRIi64 " us", micro);
            return str.c_str();
        }

        int64_t ms = micro / 1000_i64;
        return toString(str, ms);
    }

    const char* toStringNano(Str& str, int64_t nano)
    {
        str.clear();

        if (nano <= 1000_i64) {
            str.setFmt("%" PRIi64 " ns", nano);
            return str.c_str();
        }
      
        int64_t micro = nano / 1000_i64;
        return toStringMicro(str, micro);

    }

} // namespace HumanDuration

X_NAMESPACE_END;