#pragma once

#ifndef X_INVALIDPARAMETERHANDLER_H_
#define X_INVALIDPARAMETERHANDLER_H_

X_NAMESPACE_BEGIN(core)

namespace invalidParameterHandler
{
    static const DWORD EXCEPTION_CODE = 0xE000000D;

    void Startup(void);
    void Shutdown(void);

} // namespace invalidParameterHandler

X_NAMESPACE_END

#endif // X_INVALIDPARAMETERHANDLER_H_
