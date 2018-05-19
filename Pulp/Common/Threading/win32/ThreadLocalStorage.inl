

inline ThreadLocalStorage::ThreadLocalStorage(void)
{
    index_ = TlsAlloc();
    if (index_ == TLS_OUT_OF_INDEXES) {
        lastError::Description Dsc;
        X_ERROR("LocalStorage", "Failed to alloc local storage. Error: %s",
            lastError::ToString(Dsc));
    }
}

inline ThreadLocalStorage::~ThreadLocalStorage(void)
{
    if (!TlsFree(index_)) {
        lastError::Description Dsc;
        X_ERROR("LocalStorage", "Failed to free local storage idx(%d). Error: %s",
            index_, lastError::ToString(Dsc));
    }
}

inline void ThreadLocalStorage::setValue(void* value)
{
    const BOOL success = TlsSetValue(index_, value);
    if (success == 0) {
        lastError::Description Dsc;
        X_ERROR("LocalStorage", "Failed to set thread local storage idx(%d). rror: %s",
            index_, lastError::ToString(Dsc));
    }
}

inline void ThreadLocalStorage::setValueInt(intptr_t value)
{
    setValue(reinterpret_cast<void*>(value));
}


template<typename T>
inline T* ThreadLocalStorage::getValue(void) const
{
    void* data = TlsGetValue(index_);

    // see remarks at http://msdn.microsoft.com/en-us/library/windows/desktop/ms686812%28v=vs.85%29.aspx
    if (lastError::Get() != ERROR_SUCCESS) {
        lastError::Description Dsc;
        X_ERROR("LocalStorage", "Failed to get thread local storage idx(%d). Error: %s",
            index_, lastError::ToString(Dsc));
    }

    return static_cast<T*>(data);
}

inline intptr_t ThreadLocalStorage::getValueInt(void) const
{
    void* pData = getValue<void>();

    return reinterpret_cast<intptr_t>(pData);
}
