#pragma once

#ifndef X_INVALIDPARAMETERHANDLER_H_
#define X_INVALIDPARAMETERHANDLER_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Debugging
/// \brief Custom handler taking care of calls to system functions with invalid parameters.
/// \details By installing a custom handler, the invalid parameter handler takes care of intercepting all calls to system
/// functions (printf, STL container methods, etc.) with invalid parameters anywhere in the application. Whenever such a
/// function call is made, a custom exception is raised, which in turn is handled by the custom exception handler. By
/// raising an exception we get the chance to gather a call stack and write a crash dump inside the exception handler.
///
/// One prime example of how to trigger a function call with invalid parameters is by trying to access any STL container
/// with an invalid subscript, shown in the example below:
/// \code
///   std::vector<int> myVector;
///
///   // this will invoke the handler, because the index is out-of-bounds
///   int test = myVector[1];
/// \endcode
/// \sa abortHandler exceptionHandler pureVirtualFunctionCallHandler
namespace invalidParameterHandler
{
    /// A custom exception code recognized by the exception handler.
    static const DWORD EXCEPTION_CODE = 0xE000000D;

    /// \brief Starts the abort handler.
    /// \remark This is called automatically when starting the Core module.
    void Startup(void);

    /// \brief Shuts down the abort handler.
    /// \remark This is called automatically when shutting down the Core module.
    void Shutdown(void);
} // namespace invalidParameterHandler

X_NAMESPACE_END

#endif // X_INVALIDPARAMETERHANDLER_H_
