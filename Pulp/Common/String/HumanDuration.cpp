#include "EngineCommon.h"
#include "HumanDuration.h"

X_NAMESPACE_BEGIN(core)

namespace HumanDuration
{
    const char* toString(Str& str, float ms)
    {
        str.clear();

        if (ms <= 10000.f) {
            str.appendFmt("%g ms", ms);
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

} // namespace HumanDuration

X_NAMESPACE_END;