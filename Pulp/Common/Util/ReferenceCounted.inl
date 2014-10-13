
template <class T>
ReferenceCountedArena<T>::ReferenceCountedArena(void)
	: instance_()
	, refCount(1)
{
}


template <class T>
ReferenceCountedArena<T>::ReferenceCountedArena(const T& instance)
	: instance_(instance)
	, refCount_(1)
{
}


template <class T>
uint32_t ReferenceCountedArena<T>::addReference(void) const
{
	return ++refCount_;
}


template <class T>
uint32_t ReferenceCountedArena<T>::removeReference(void) const
{
	return --refCount_;
}


template <class T>
T* ReferenceCountedArena<T>::getInstance(void)
{
	return &instance_;
}


template <class T>
const T* ReferenceCountedArena<T>::getInstance(void) const
{
	return &instance_;
}


// -------------------------------------------------------------
// -------------------------------------------------------------

template<typename T>
ReferenceCounted<T>::ReferenceCounted() :
refCount_(0)
{

}

template<typename T>
ReferenceCounted<T>::~ReferenceCounted()
{
	X_ASSERT(refCount_ == 0, "refcounted object refe count is not zero")(refCount_);
}

template<typename T>
void ReferenceCounted<T>::addRef(void)
{
	refCount_++;
}

template<typename T>
uint32_t ReferenceCounted<T>::release(void)
{
	return --refCount_;
}
