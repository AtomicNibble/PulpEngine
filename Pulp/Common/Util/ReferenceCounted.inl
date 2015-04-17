
template <class T>
ReferenceCountedArena<T>::ReferenceCountedArena(void)
	: refCount_(1)
{
}


template <class T>
ReferenceCountedArena<T>::ReferenceCountedArena(const T& instance)
	: refCount_(1)
{
}


template <class T>
uint32_t ReferenceCountedArena<T>::addReference(void)
{
	return ++refCount_;
}


template <class T>
uint32_t ReferenceCountedArena<T>::removeReference(void)
{
	return --refCount_;
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
