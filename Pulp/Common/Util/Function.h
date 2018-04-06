#pragma once

#include <functional>
#include <memory>

X_NAMESPACE_BEGIN(core)

template<class, size_t MaxSize = 256>
class Function;

template<class R, class... Args, size_t MaxSize>
class Function<R(Args...), MaxSize>
{
    enum class Operation
    {
        Clone,
        Destroy
    };

    using Invoker = R (*)(void*, Args&&...);
    using Manager = void (*)(void*, void*, Operation);
    using Storage = typename std::aligned_storage<MaxSize - sizeof(Invoker) - sizeof(Manager), 8>::type;

public:
    X_INLINE Function();
    X_INLINE Function(std::nullptr_t);

    X_INLINE Function(const Function& other);
    X_INLINE Function(Function&& other);

    template<class F>
    X_INLINE Function(F&& f);

    X_INLINE ~Function();

    X_INLINE Function& operator=(const Function& other);
    X_INLINE Function& operator=(Function&& other);
    X_INLINE Function& operator=(std::nullptr_t);

    template<typename F>
    X_INLINE Function& operator=(F&& f);
    template<typename F>
    X_INLINE Function& operator=(std::reference_wrapper<F> f);

    X_INLINE explicit operator bool() const;
    X_INLINE R operator()(Args... args);
    X_INLINE R Invoke(Args... args);

    X_INLINE void swap(Function& other);

private:
    template<typename F>
    X_INLINE static R invoke(void* data, Args&&... args)
    {
        F& f = *static_cast<F*>(data);
        return f(std::forward<Args>(args)...);
    }

    template<typename F>
    X_INLINE static void manage(void* dest, void* src, Operation op)
    {
        switch (op) {
            case Operation::Clone:
                new (dest) F(*static_cast<F*>(src));
                break;
            case Operation::Destroy:
                static_cast<F*>(dest)->~F();
                break;
        }
    }

private:
    Invoker invoker_;
    Manager manager_;
    Storage data_;
};

X_NAMESPACE_END

#include "Function.inl"