#include "EngineCommon.h"

#include "AbortHandler.h"
#include "ICore.h"

#include <signal.h>

X_NAMESPACE_BEGIN(core)

namespace abortHandler
{
    namespace
    {
        void (*g_oldAbortHandler)(int);

        void _AbortHandler(int __formal)
        {
            X_FATAL("AbortHandler", "abort() has been called.");
            X_UNUSED(__formal);

            RaiseException(EXCEPTION_CODE, 0, 0, 0);
        }

    } // namespace

    void Startup(void)
    {
        //	X_LOG0( "AbortHandler", "Registering abort handler." );

        g_oldAbortHandler = signal(SIGABRT, _AbortHandler);
    }

    void Shutdown(void)
    {
        //	X_LOG0( "AbortHandler", "Unregistering abort handler." );

        signal(SIGABRT, g_oldAbortHandler);
    }

} // namespace abortHandler

X_NAMESPACE_END