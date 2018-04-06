#include "EngineCommon.h"

#include "InvalidParameterHandler.h"
#include "String\StringUtil.h"

#include "DebuggerConnection.h"

#include <stdlib.h>

#include "ICore.h"

X_NAMESPACE_BEGIN(core)

namespace invalidParameterHandler
{
    namespace
    {
        _invalid_parameter_handler g_oldHandler;

        void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file,
            unsigned int line, uintptr_t pReserved)
        {
            X_FATAL("InvParamHandler", "A CRT function has been called with an invalid parameter.");
            X_UNUSED(pReserved);

            {
                X_LOG_BULLET;

                char convFile[256];
                char convFunction[256];
                char convExpression[256];

                memset(convExpression, 0, 0x100u);
                memset(convFunction, 0, 0x100u);
                memset(convFile, 0, 0x100u);
                strUtil::Convert(expression, convExpression, 0x100u);
                strUtil::Convert(function, convFunction, 0x100u);
                strUtil::Convert(file, convFile, 0x100u);

                X_ERROR("InvParamHandler", "Failed expression: %s", convExpression);
                X_ERROR("InvParamHandler", "Function: %s", function);
                X_ERROR("InvParamHandler", "File: %s", file);
                X_ERROR("InvParamHandler", "Line: %i", line);
            }
            RaiseException(EXCEPTION_CODE, 0, 0, 0);
        }

    } // namespace

    void Startup(void)
    {
        if (!debugging::IsDebuggerConnected()) {
            g_oldHandler = _set_invalid_parameter_handler(myInvalidParameterHandler);

            if (g_oldHandler != myInvalidParameterHandler) {
                X_LOG0("InvParamHandler", "Registering invalid parameter handler.");
            }
        }
    }

    void Shutdown(void)
    {
        //	X_LOG0( "InvParamHandler", "Unregistering invalid parameter handler." );

        if (!debugging::IsDebuggerConnected()) {
            _set_invalid_parameter_handler(g_oldHandler);
        }
    }

} // namespace invalidParameterHandler

X_NAMESPACE_END