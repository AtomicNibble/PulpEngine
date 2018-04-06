#pragma once

#ifndef X_ABORTHANDLER_H_
#define X_ABORTHANDLER_H_

X_NAMESPACE_BEGIN(core)

namespace abortHandler
{
    /// A custom exception code recognized by the exception handler.
    static const DWORD EXCEPTION_CODE = 0xE000000F;

    /// \brief Starts the abort handler.
    /// \remark This is called automatically when starting the Core module.
    void Startup(void);

    /// \brief Shuts down the abort handler.
    /// \remark This is called automatically when shutting down the Core module.
    void Shutdown(void);
} // namespace abortHandler

X_NAMESPACE_END

#endif
