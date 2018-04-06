

X_NAMESPACE_BEGIN(core)

template<typename T, class ContainerT, class Pr>
PriorityQueue<T, ContainerT, Pr>::PriorityQueue(core::MemoryArenaBase* arena) :
    container_(arena),
    comp_()
{
}

template<typename T, class ContainerT, class Pr>
PriorityQueue<T, ContainerT, Pr>::PriorityQueue(const MyType& oth) :
    container_(oth.container_),
    comp_(oth.comp_)
{
}

template<typename T, class ContainerT, class Pr>
PriorityQueue<T, ContainerT, Pr>::PriorityQueue(MyType&& oth) :
    container_(std::move(oth.container_)),
    comp_(std::move(oth.comp_))
{
}

template<typename T, class ContainerT, class Pr>
typename PriorityQueue<T, ContainerT, Pr>::MyType& PriorityQueue<T, ContainerT, Pr>::operator=(const MyType& rhs)
{
    container_ = rhs.container_;
    comp_ = rhs.comp_;
    return *this;
}

template<typename T, class ContainerT, class Pr>
typename PriorityQueue<T, ContainerT, Pr>::MyType& PriorityQueue<T, ContainerT, Pr>::operator=(MyType&& rhs)
{
    container_ = std::move(rhs.container_);
    comp_ = std::move(rhs.comp_);
    return *this;
}

template<typename T, class ContainerT, class Pr>
void PriorityQueue<T, ContainerT, Pr>::clear(void)
{
    container_.clear();
}

template<typename T, class ContainerT, class Pr>
void PriorityQueue<T, ContainerT, Pr>::free(void)
{
    container_.free();
}

template<typename T, class ContainerT, class Pr>
void PriorityQueue<T, ContainerT, Pr>::push(const value_type& val)
{
    container_.push_back(val);
    std::push_heap(container_.begin(), container_.end(), comp_);
}

template<typename T, class ContainerT, class Pr>
void PriorityQueue<T, ContainerT, Pr>::push(value_type&& val)
{
    container_.push_back(std::move(val));
    std::push_heap(container_.begin(), container_.end(), comp_);
}

template<typename T, class ContainerT, class Pr>
template<class... _Valty>
void PriorityQueue<T, ContainerT, Pr>::emplace(_Valty&&... val)
{
    container_.emplace_back(std::forward<_Valty>(val)...);
    std::push_heap(container_.begin(), container_.end(), comp_);
}

template<typename T, class ContainerT, class Pr>
void PriorityQueue<T, ContainerT, Pr>::pop(void)
{
    std::pop_heap(container_.begin(), container_.end(), comp_);
    container_.pop_back();
}

template<typename T, class ContainerT, class Pr>
typename PriorityQueue<T, ContainerT, Pr>::const_reference PriorityQueue<T, ContainerT, Pr>::peek(void) const
{
    return container_.front();
}

template<typename T, class ContainerT, class Pr>
bool PriorityQueue<T, ContainerT, Pr>::isEmpty(void) const
{
    return container_.isEmpty();
}

template<typename T, class ContainerT, class Pr>
bool PriorityQueue<T, ContainerT, Pr>::isNotEmpty(void) const
{
    return container_.isNotEmpty();
}

template<typename T, class ContainerT, class Pr>
typename PriorityQueue<T, ContainerT, Pr>::size_type PriorityQueue<T, ContainerT, Pr>::size(void) const
{
    return container_.size();
}

template<typename T, class ContainerT, class Pr>
typename PriorityQueue<T, ContainerT, Pr>::size_type PriorityQueue<T, ContainerT, Pr>::capacity(void) const
{
    return container_.capacity();
}

X_NAMESPACE_END