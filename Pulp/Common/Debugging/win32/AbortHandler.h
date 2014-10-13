#pragma once

#ifndef ME_ABORTHANDLER_H_
#define ME_ABORTHANDLER_H_


X_NAMESPACE_BEGIN(core)

/// \ingroup Debugging
/// \brief Custom signal handler taking care of calls to abort().
/// \details By installing a custom signal handler, the abort handler takes care of intercepting all calls to abort()
/// anywhere in the application. Whenever abort() is called, a custom exception is raised, which in turn is handled by the
/// custom exception handler. By raising an exception we get the chance to gather a call stack and write a crash dump
/// inside the exception handler.
/// \sa exceptionHandler pureVirtualFunctionCallHandler invalidParameterHandler
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
}

X_NAMESPACE_END


#endif
