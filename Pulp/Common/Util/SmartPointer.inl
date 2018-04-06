

template<typename T>
SmartPointer<T>::SmartPointer() :
    pData_(nullptr)
{
}

template<typename T>
SmartPointer<T>::SmartPointer(T* p) :
    pData_(p)
{
    if (p) {
        p->addRef();
    }
}

template<typename T>
SmartPointer<T>::SmartPointer(const SmartPointer& oth) :
    pData_(oth.pData_)
{
    if (pData_) {
        pData_->addRef();
    }
}

template<typename T>
SmartPointer<T>::~SmartPointer()
{
    if (pData_) {
        pData_->release();
    }
}

template<typename T>
SmartPointer<T>::operator T*() const
{
    return pData_;
}

template<typename T>
SmartPointer<T>::operator const T*() const
{
    return pData_;
}

template<typename T>
SmartPointer<T>::operator bool() const
{
    return pData_;
}

template<typename T>
T& SmartPointer<T>::operator*() const
{
    return &pData_;
}

template<typename T>
T* SmartPointer<T>::operator->() const
{
    return pData_;
}

template<typename T>
T* SmartPointer<T>::get() const
{
    return pData_;
}

template<typename T>
void SmartPointer<T>::reset()
{
    SmartPointer<T>().swap(*this);
}

template<typename T>
void SmartPointer<T>::reset(T* p)
{
    SmartPointer<T>(p).swap(*this);
}

template<typename T>
SmartPointer<T>& SmartPointer<T>::operator=(T* newP)
{
    if (newP) {
        newP->addRef();
    }
    if (pData_) {
        pData_->release();
    }
    pData_ = newP;
    return *this;
}

template<typename T>
SmartPointer<T>& SmartPointer<T>::operator=(const SmartPointer<T>& newp)
{
    if (newp.pData_) {
        newp.pData_->addRef();
    }
    if (pData_) {
        pData_->release();
    }
    pData_ = newp.pData_;
    return *this;
}

template<typename T>
bool SmartPointer<T>::operator!() const
{
    return pData_ != nullptr;
}

template<typename T>
bool SmartPointer<T>::operator==(const T* p2) const
{
    return pData_ == p2;
}

template<typename T>
bool SmartPointer<T>::operator==(T* p2) const
{
    return pData_ == p2;
}

template<typename T>
bool SmartPointer<T>::operator==(const SmartPointer<T>& rhs) const
{
    return pData_ == rhs.pData_;
}

template<typename T>
bool SmartPointer<T>::operator!=(const T* p2) const
{
    return !(*this == p2);
}

template<typename T>
bool SmartPointer<T>::operator!=(T* p2) const
{
    return !(*this == p2);
}

template<typename T>
bool SmartPointer<T>::operator!=(const SmartPointer& p2) const
{
    return !(*this == p2);
}

template<typename T>
bool SmartPointer<T>::operator<(const T* p2) const
{
    return pData_ < p2;
}

template<typename T>
bool SmartPointer<T>::operator>(const T* p2) const
{
    return pData_ > p2;
}

template<typename T>
void SmartPointer<T>::swap(SmartPointer<T>& oth)
{
    core::Swap(pData_, oth.pData_);
}
