#pragma once

#ifndef _X_THREADING_LOCAL_STORAGE_H_
#define _X_THREADING_LOCAL_STORAGE_H_

#include <Util/LastError.h>

X_NAMESPACE_BEGIN(core)

/// \code
///   core::ThreadLocalStorage tlsTest;
///
///   // assume this function is called from several different threads
///   void Work(void)
///   {
///     // get the data associated with the current thread from thread local storage
///     DataStructure* data = tlsTest.GetValue<DataStructure>();
///     ...
///   }
///
///   core::Thread::ReturnValue ThreadFunction(const core::Thread& thread)
///   {
///     DataStructure* data = new DataStructure(...);
///
///     // add this thread's data to the thread local storage
///     tlsTest.SetValue(data);
///
///     // do something
///     ...
///     Work();
///     ...
///
///     delete data;
///   	return core::Thread::ReturnValue(0);
///   }
/// \endcode

class ThreadLocalStorage
{
public:
    /// Allocate a thread local storage slot from the OS.
    inline ThreadLocalStorage(void);
    inline ~ThreadLocalStorage(void);

    /// Associate data with the calling thread.
    inline void SetValue(void* value);
    inline void SetValueInt(intptr_t value);

    /// Retrieve data from thread local storage for the calling thread.
    template<typename T>
    inline T* GetValue(void) const;
    inline intptr_t GetValueInt(void) const;


private:
    X_NO_COPY(ThreadLocalStorage);
    X_NO_ASSIGN(ThreadLocalStorage);

    DWORD index_;
};

#include "ThreadLocalStorage.inl"

X_NAMESPACE_END

#endif // !_X_THREADING_LOCAL_STORAGE_H_
