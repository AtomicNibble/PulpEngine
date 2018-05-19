#pragma once

#ifndef _X_THREADING_LOCAL_STORAGE_H_
#define _X_THREADING_LOCAL_STORAGE_H_

#include <Util/LastError.h>

X_NAMESPACE_BEGIN(core)


class ThreadLocalStorage
{
public:
    /// Allocate a thread local storage slot from the OS.
    inline ThreadLocalStorage(void);
    inline ~ThreadLocalStorage(void);

    /// Associate data with the calling thread.
    inline void setValue(void* value);
    inline void setValueInt(intptr_t value);

    /// Retrieve data from thread local storage for the calling thread.
    template<typename T>
    inline T* getValue(void) const;
    inline intptr_t getValueInt(void) const;


private:
    X_NO_COPY(ThreadLocalStorage);
    X_NO_ASSIGN(ThreadLocalStorage);

    DWORD index_;
};

#include "ThreadLocalStorage.inl"

X_NAMESPACE_END

#endif // !_X_THREADING_LOCAL_STORAGE_H_
