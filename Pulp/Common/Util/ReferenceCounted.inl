
template <class T>
ReferenceCountedInstance<T>::ReferenceCountedInstance(void)
	: instance_(),
	refCount_(1)
{
}


template <class T>
ReferenceCountedInstance<T>::ReferenceCountedInstance(const T& instance)
	: instance_(instance),
	refCount_(1)
{
}


template <class T>
uint32_t ReferenceCountedInstance<T>::addReference(void)
{
	return ++refCount_;
}


template <class T>
uint32_t ReferenceCountedInstance<T>::removeReference(void)
{
	return --refCount_;
}


template <class T>
T* ReferenceCountedInstance<T>::instance(void)
{
	return &instance_;
}

template <class T>
const T* ReferenceCountedInstance<T>::instance(void) const
{
	return &instance_;
}


// ======================================


template <class T>
ReferenceCounted<T>::ReferenceCounted(void) :
	refCount_(1)
{
}

template <class T>
X_INLINE uint32_t ReferenceCounted<T>::addReference(void)
{
	return ++refCount_;
}
	
template <class T>	
X_INLINE uint32_t ReferenceCounted<T>::removeReference(void)
{
	return --refCount_;
}

template <class T>
X_INLINE uint32_t ReferenceCounted<T>::getRefCount(void) const
{
	return refCount_;
}