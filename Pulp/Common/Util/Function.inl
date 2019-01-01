#pragma once

X_NAMESPACE_BEGIN(core)

template<class R, class... Args, size_t MaxSize>
Function<R(Args...), MaxSize>::Function() :
    invoker_(nullptr),
    manager_(nullptr)
{
}

template<class R, class... Args, size_t MaxSize>
Function<R(Args...), MaxSize>::Function(std::nullptr_t) :
    invoker_(nullptr),
    manager_(nullptr)
{
}

template<class R, class... Args, size_t MaxSize>
Function<R(Args...), MaxSize>::Function(const Function& other)
{
    if (other) {
        other.manager_(&data_, &other.data_, Operation::Clone);
        invoker_ = other.invoker_;
        manager_ = other.manager_;
    }
}

template<class R, class... Args, size_t MaxSize>
Function<R(Args...), MaxSize>::Function(Function&& other) :
    invoker_(nullptr),
    manager_(nullptr)
{
    other.swap(*this);
}

template<class R, class... Args, size_t MaxSize>
template<class F>
Function<R(Args...), MaxSize>::Function(F&& f)
{
    using f_type = typename std::decay<F>::type;

    static_assert(alignof(f_type) <= alignof(Storage), "invalid alignment");
    static_assert(sizeof(f_type) <= sizeof(Storage), "storage too small");

    new (&data_) f_type(std::forward<F>(f));

    invoker_ = &invoke<f_type>;
    manager_ = &manage<f_type>;
}

template<class R, class... Args, size_t MaxSize>
Function<R(Args...), MaxSize>::~Function()
{
    if (manager_) {
        manager_(&data_, nullptr, Operation::Destroy);
    }
}

template<class R, class... Args, size_t MaxSize>
typename Function<R(Args...), MaxSize>::Function& Function<R(Args...), MaxSize>::operator=(const Function& other)
{
    Function(other).swap(*this);
    return *this;
}

template<class R, class... Args, size_t MaxSize>
typename Function<R(Args...), MaxSize>::Function& Function<R(Args...), MaxSize>::operator=(Function&& other)
{
    Function(std::move(other)).swap(*this);
    return *this;
}

template<class R, class... Args, size_t MaxSize>
typename Function<R(Args...), MaxSize>::Function& Function<R(Args...), MaxSize>::operator=(std::nullptr_t)
{
    if (manager_) {
        manager_(&data_, nullptr, Operation::Destroy);
        manager_ = nullptr;
        invoker_ = nullptr;
    }
    return *this;
}

template<class R, class... Args, size_t MaxSize>
template<typename F>
typename Function<R(Args...), MaxSize>::Function& Function<R(Args...), MaxSize>::operator=(F&& f)
{
    Function(std::forward<F>(f)).swap(*this);
    return *this;
}

template<class R, class... Args, size_t MaxSize>
template<typename F>
typename Function<R(Args...), MaxSize>::Function& Function<R(Args...), MaxSize>::operator=(std::reference_wrapper<F> f)
{
    Function(f).swap(*this);
    return *this;
}

template<class R, class... Args, size_t MaxSize>
Function<R(Args...), MaxSize>::operator bool() const
{
    return !!manager_;
}

template<class R, class... Args, size_t MaxSize>
R Function<R(Args...), MaxSize>::operator()(Args... args)
{
    if (!invoker_) {
        // worth the branch?
        // should just let it fail :D
    }
    return invoker_(&data_, std::forward<Args>(args)...);
}

template<class R, class... Args, size_t MaxSize>
R Function<R(Args...), MaxSize>::Invoke(Args... args)
{
    return invoker_(&data_, std::forward<Args>(args)...);
}

template<class R, class... Args, size_t MaxSize>
void Function<R(Args...), MaxSize>::swap(Function& other)
{
    core::Swap(data_, other.data_);
    core::Swap(manager_, other.manager_);
    core::Swap(invoker_, other.invoker_);
}

X_NAMESPACE_END
