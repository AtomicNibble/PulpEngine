
template <class T>
ReferenceCountedOwner<T>::ReferenceCountedOwner(T* instance, MemoryArenaBase* arena)
	: instance_(instance)
	, arena_(arena)
{
	instance_->addReference();
}


template <class T>
ReferenceCountedOwner<T>::ReferenceCountedOwner(const ReferenceCountedOwner<T>& other)
	: instance_(other.instance_)
	, arena_(other.arena_)
{
	instance_->addReference();
}


template <class T>
ReferenceCountedOwner<T>& ReferenceCountedOwner<T>::operator=(const ReferenceCountedOwner<T>& other)
{
	if (this != &other)
	{
		instance_ = other.instance_;
		instance_->addReference();

		arena_ = other.arena_;
	}

	return *this;
}


template <class T>
ReferenceCountedOwner<T>::~ReferenceCountedOwner(void)
{
	// delete the instance as soon as its reference count reaches zero
	if (instance_->removeReference() == 0)
	{
		X_DELETE(instance_, arena_);
	}
}


template <class T>
T* ReferenceCountedOwner<T>::operator->(void)
{
	return instance_;
}


template <class T>
const T* ReferenceCountedOwner<T>::operator->(void) const
{
	return instance_;
}

template <class T>
T* ReferenceCountedOwner<T>::instance(void)
{
	return instance_;
}

template <class T>
const T* ReferenceCountedOwner<T>::instance(void) const
{
	return instance_;
}