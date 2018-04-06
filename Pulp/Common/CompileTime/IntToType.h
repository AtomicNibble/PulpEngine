
#pragma once
#ifndef X_INTTOTYPE_H_
#define X_INTTOTYPE_H_

X_NAMESPACE_BEGIN(core)

namespace compileTime
{
    template<int N>
    struct IntToType
    {
        static const int Value = N;
    };
} // namespace compileTime

X_NAMESPACE_END

#endif // X_INTTOTYPE_H_
