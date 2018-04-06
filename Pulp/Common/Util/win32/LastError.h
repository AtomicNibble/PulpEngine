#pragma once

#ifndef X_LASTERROR_H
#define X_LASTERROR_H

X_NAMESPACE_BEGIN(core)

namespace lastError
{
    typedef char Description[512];

    unsigned int Get(void);

    const char* ToString(DWORD error, Description& desc);

    const char* ToString(Description& desc);
} // namespace lastError

X_NAMESPACE_END

#endif
