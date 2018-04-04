
#pragma once
#ifndef X_EXTRACTCOUNT_H_
#define X_EXTRACTCOUNT_H_

X_NAMESPACE_BEGIN(core)

namespace compileTime
{
    template<class T>
    struct ExtractCount
    {
        static const size_t Value = 1;
    };

    template<class T, size_t N>
    struct ExtractCount<T[N]>
    {
        static const size_t Value = N;
    };

} // namespace compileTime

X_NAMESPACE_END

#endif // X_EXTRACTCOUNT_H_
