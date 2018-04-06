#pragma once

#ifndef X_PUREVIRTUALFUNCTIONCALLHANDLER_H_
#define X_PUREVIRTUALFUNCTIONCALLHANDLER_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Debugging
/// \brief Custom handler taking care of pure virtual function calls.
/// \details By installing a custom handler, the pure virtual function call handler takes care of intercepting all calls
/// to pure virtual functions anywhere in the application. Whenever such a function is called, a custom exception is
/// raised, which in turn is handled by the custom exception handler. By raising an exception we get the chance to gather
/// a call stack and write a crash dump inside the exception handler.
///
/// Even though C++ forbids calling a pure virtual function directly, be aware that this can still happen e.g. when
/// calling virtual functions from a constructor or destructor, as shown in the following example:
/// \code
///   struct Base
///   {
///       Base(void)
///       {
///         // triggers a pure virtual function call
///         Call();
///       }
///
///       void Call(void)
///       {
///           Do();
///       }
///
///       virtual void Do(void) = 0;
///   };
///
///   struct Derived : public Base
///   {
///       virtual void Do(void)
///       {
///       }
///   };
///
///   // tries to call a pure virtual function
///   Base* base = new Derived;
/// \endcode
/// Most compilers will not emit a warning whenever the virtual function is not called directly, as in the example above.
/// \sa abortHandler exceptionHandler invalidParameterHandler
namespace pureVirtualFunctionCallHandler
{
    /// A custom exception code recognized by the exception handler.
    static const DWORD EXCEPTION_CODE = 0xE000000E;

    /// \brief Starts the pure virtual function call handler.
    /// \remark This is called automatically when starting the Core module.
    void Startup(void);

    /// \brief Shuts down the pure virtual function call handler.
    /// \remark This is called automatically when shutting down the Core module.
    void Shutdown(void);
} // namespace pureVirtualFunctionCallHandler

X_NAMESPACE_END

#endif
