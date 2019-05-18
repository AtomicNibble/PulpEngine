#pragma once

#ifndef X_LASTERROR_H
#define X_LASTERROR_H

X_NAMESPACE_BEGIN(core)

namespace lastError
{
    typedef char Description[512];

    unsigned int Get(void);

    X_NO_INLINE const char* ToString(DWORD error, Description& desc);
    X_NO_INLINE const char* ToString(Description& desc);

} // namespace lastError

X_NAMESPACE_END

#endif
