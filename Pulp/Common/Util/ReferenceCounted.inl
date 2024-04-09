
template<class T, typename Primitive>
ReferenceCountedInstance<T, Primitive>::ReferenceCountedInstance(void) :
    instance_(),
    refCount_(1)
{
}

template<class T, typename Primitive>
ReferenceCountedInstance<T, Primitive>::ReferenceCountedInstance(const T& instance) :
    instance_(instance),
    refCount_(1)
{
}

template<class T, typename Primitive>
int32_t ReferenceCountedInstance<T, Primitive>::addReference(void) const
{
    return ++refCount_;
}

template<class T, typename Primitive>
int32_t ReferenceCountedInstance<T, Primitive>::removeReference(void) const
{
    return --refCount_;
}

template<class T, typename Primitive>
int32_t ReferenceCountedInstance<T, Primitive>::getRefCount(void) const
{
    return refCount_;
}

template<class T, typename Primitive>
T* ReferenceCountedInstance<T, Primitive>::instance(void)
{
    return &instance_;
}

template<class T, typename Primitive>
const T* ReferenceCountedInstance<T, Primitive>::instance(void) const
{
    return &instance_;
}

// ======================================

template<class T, typename Primitive>
int32_t ReferenceCountedInherit<T, Primitive>::addReference(void) const
{
    return ++refCount_;
}

template<class T, typename Primitive>
int32_t ReferenceCountedInherit<T, Primitive>::removeReference(void) const
{
    return --refCount_;
}

template<class T, typename Primitive>
int32_t ReferenceCountedInherit<T, Primitive>::getRefCount(void) const
{
    return refCount_;
}

template<class T, typename Primitive>
T* ReferenceCountedInherit<T, Primitive>::instance(void)
{
    return this;
}

template<class T, typename Primitive>
const T* ReferenceCountedInherit<T, Primitive>::instance(void) const
{
    return this;
}

// ======================================

template<typename Primitive>
ReferenceCounted<Primitive>::ReferenceCounted(void) :
    refCount_(1)
{
}

template<typename Primitive>
X_INLINE int32_t ReferenceCounted<Primitive>::addReference(void) const
{
    return ++refCount_;
}

template<typename Primitive>
X_INLINE int32_t ReferenceCounted<Primitive>::removeReference(void) const
{
    return --refCount_;
}

template<typename Primitive>
X_INLINE int32_t ReferenceCounted<Primitive>::getRefCount(void) const
{
    return refCount_;
}