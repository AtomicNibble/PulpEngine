
template<class T>
ReferenceCountedOwner<T>::ReferenceCountedOwner(T* instance, MemoryArenaBase* arena) :
    instance_(instance),
    arena_(arena)
{
    // makes the refrence counting not work correct.
    // a instance should start with ref count of 1.
    // so doing this makes it 2, making the UnitTests fail.
    // blame commit: bd324b210cb7a23f7903590e1b2f7be3d860c37d
    // Reason this was added:
    // I added this before so that you could create multiple instances of CountedOwer and it would
    // increase the ref count yet i could still pass the pointer around.
    //
    //instance_->addReference();
}

template<class T>
ReferenceCountedOwner<T>::ReferenceCountedOwner(const ReferenceCountedOwner<T>& other) :
    instance_(other.instance_),
    arena_(other.arena_)
{
    instance_->addReference();
}

template<class T>
X_INLINE ReferenceCountedOwner<T>::ReferenceCountedOwner(ReferenceCountedOwner<T>&& other) :
    instance_(other.instance_),
    arena_(other.arena_)
{
    other.instance_ = nullptr;
}

template<class T>
ReferenceCountedOwner<T>& ReferenceCountedOwner<T>::operator=(const ReferenceCountedOwner<T>& other)
{
    if (this != &other) {
        if (instance_ && instance_->removeReference() == 0) {
            X_DELETE(instance_, arena_);
        }

        instance_ = other.instance_;
        if (instance_) {
            instance_->addReference();
        }
        arena_ = other.arena_;
    }

    return *this;
}

template<class T>
ReferenceCountedOwner<T>& ReferenceCountedOwner<T>::operator=(ReferenceCountedOwner<T>&& other)
{
    instance_ = other.instance_;
    arena_ = other.arena_;

    other.instance_ = nullptr;
    return *this;
}

template<class T>
ReferenceCountedOwner<T>::~ReferenceCountedOwner(void)
{
    // delete the instance as soon as its reference count reaches zero
    if (instance_ && instance_->removeReference() == 0) {
        X_DELETE(instance_, arena_);
    }
}

template<class T>
T* ReferenceCountedOwner<T>::operator->(void)
{
    return instance_;
}

template<class T>
const T* ReferenceCountedOwner<T>::operator->(void)const
{
    return instance_;
}

template<class T>
T* ReferenceCountedOwner<T>::instance(void)
{
    return instance_;
}

template<class T>
const T* ReferenceCountedOwner<T>::instance(void) const
{
    return instance_;
}