#pragma once

#ifndef X_ISSTRINGLITERAL_H_
#define X_ISSTRINGLITERAL_H_

X_NAMESPACE_BEGIN(core)

namespace compileTime
{
    template<typename T>
    struct IsStringLiteral
    {
        static const bool Value = false;
    };

    template<size_t N>
    struct IsStringLiteral<const char[N]>
    {
        static const bool Value = true;
    };

    template<size_t N>
    struct IsStringLiteral<const char (&)[N]>
    {
        static const bool Value = true;
    };

} // namespace compileTime

X_NAMESPACE_END

#endif
