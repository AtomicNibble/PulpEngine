#include "EngineCommon.h"

#include "PureVirtualFunctionCallHandler.h"
#include "ICore.h"

#include <stdlib.h>

X_NAMESPACE_BEGIN(core)

namespace pureVirtualFunctionCallHandler
{
    namespace
    {
        void (*g_oldHandler)();

        void _PureVirtualFunctionCallHandler()
        {
            X_FATAL("PureVfcHandler", "A pure virtual function has been called.");

            RaiseException(EXCEPTION_CODE, 0, 0, 0);
        }
    } // namespace

    /// \brief Starts the pure virtual function call handler.
    /// \remark This is called automatically when starting the Core module.
    void Startup(void)
    {
        //	X_LOG0( "PureVfcHandler", "Registering pure virtual function call handler." );

        g_oldHandler = _set_purecall_handler(_PureVirtualFunctionCallHandler);
    }

    /// \brief Shuts down the pure virtual function call handler.
    /// \remark This is called automatically when shutting down the Core module.
    void Shutdown(void)
    {
        //	X_LOG0( "PureVfcHandler", "Unregistering pure virtual function call handler." );

        _set_purecall_handler(g_oldHandler);
    }

} // namespace pureVirtualFunctionCallHandler

X_NAMESPACE_END