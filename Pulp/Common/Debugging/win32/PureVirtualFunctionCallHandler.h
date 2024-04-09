#pragma once

#ifndef X_PUREVIRTUALFUNCTIONCALLHANDLER_H_
#define X_PUREVIRTUALFUNCTIONCALLHANDLER_H_

X_NAMESPACE_BEGIN(core)

namespace pureVirtualFunctionCallHandler
{
    static const DWORD EXCEPTION_CODE = 0xE000000E;

    // Starts the pure virtual function call handler.
    // This is called automatically when starting the Core module.
    void Startup(void);

    // Shuts down the pure virtual function call handler.
    // This is called automatically when shutting down the Core module.
    void Shutdown(void);

} // namespace pureVirtualFunctionCallHandler

X_NAMESPACE_END

#endif
