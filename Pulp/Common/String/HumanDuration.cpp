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
        else if (ms <= 300000) {
            const float sec = (ms / 1000);
            str.appendFmt("%.2f sec", sec);
        }
        else if (ms <= 16777216) {
            const float sec = (ms / 1000);
            const float min = (sec / 60);

            str.appendFmt("%.2f min", min);
        }
        else {
            const float sec = (ms / 1000);
            const float min = (sec / 60);

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
            const int64_t sec = (ms / 1000);
            str.appendFmt("%" PRIi64 " sec", sec);
        }
        else if (ms <= 16777216_i64) {
            const int64_t sec = (ms / 1000);
            const int64_t min = (sec / 60);

            str.appendFmt("%" PRIi64 " min", min);
        }
        else {
            const int64_t sec = (ms / 1000);
            const int64_t min = (sec / 60);

            str.appendFmt("%" PRIi64 " min", min);
        }

        return str.c_str();
    }

} // namespace HumanDuration

X_NAMESPACE_END;