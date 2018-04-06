
template<class T, typename Primative>
ReferenceCountedInstance<T, Primative>::ReferenceCountedInstance(void) :
    instance_(),
    refCount_(1)
{
}

template<class T, typename Primative>
ReferenceCountedInstance<T, Primative>::ReferenceCountedInstance(const T& instance) :
    instance_(instance),
    refCount_(1)
{
}

template<class T, typename Primative>
int32_t ReferenceCountedInstance<T, Primative>::addReference(void) const
{
    return ++refCount_;
}

template<class T, typename Primative>
int32_t ReferenceCountedInstance<T, Primative>::removeReference(void) const
{
    return --refCount_;
}

template<class T, typename Primative>
int32_t ReferenceCountedInstance<T, Primative>::getRefCount(void) const
{
    return refCount_;
}

template<class T, typename Primative>
T* ReferenceCountedInstance<T, Primative>::instance(void)
{
    return &instance_;
}

template<class T, typename Primative>
const T* ReferenceCountedInstance<T, Primative>::instance(void) const
{
    return &instance_;
}

// ======================================

template<class T, typename Primative>
int32_t ReferenceCountedInherit<T, Primative>::addReference(void) const
{
    return ++refCount_;
}

template<class T, typename Primative>
int32_t ReferenceCountedInherit<T, Primative>::removeReference(void) const
{
    return --refCount_;
}

template<class T, typename Primative>
int32_t ReferenceCountedInherit<T, Primative>::getRefCount(void) const
{
    return refCount_;
}

template<class T, typename Primative>
T* ReferenceCountedInherit<T, Primative>::instance(void)
{
    return this;
}

template<class T, typename Primative>
const T* ReferenceCountedInherit<T, Primative>::instance(void) const
{
    return this;
}

// ======================================

template<typename Primative>
ReferenceCounted<Primative>::ReferenceCounted(void) :
    refCount_(1)
{
}

template<typename Primative>
X_INLINE int32_t ReferenceCounted<Primative>::addReference(void) const
{
    return ++refCount_;
}

template<typename Primative>
X_INLINE int32_t ReferenceCounted<Primative>::removeReference(void) const
{
    return --refCount_;
}

template<typename Primative>
X_INLINE int32_t ReferenceCounted<Primative>::getRefCount(void) const
{
    return refCount_;
}