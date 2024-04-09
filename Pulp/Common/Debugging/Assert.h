#pragma once

#ifndef X_ASSERT_H_
#define X_ASSERT_H_

#include "Util/SourceInfo.h"

X_NAMESPACE_BEGIN(core)

class Assert
{
public:
    // Constructs an instance, dispatching the source info and the formatted messages using logDispatch and assertionDispatch.
    Assert(const SourceInfo& sourceInfo, const char* format, ...);

    Assert& Variable(const char* const name, bool var);
    Assert& Variable(const char* const name, char var);
    Assert& Variable(const char* const name, signed char var);
    Assert& Variable(const char* const name, unsigned char var);
    Assert& Variable(const char* const name, short var);
    Assert& Variable(const char* const name, unsigned short var);
    Assert& Variable(const char* const name, int var);
    Assert& Variable(const char* const name, unsigned int var);
    Assert& Variable(const char* const name, long var);
    Assert& Variable(const char* const name, unsigned long var);
    Assert& Variable(const char* const name, long long var);
    Assert& Variable(const char* const name, unsigned long long var);
    Assert& Variable(const char* const name, float var);
    Assert& Variable(const char* const name, double var);
    Assert& Variable(const char* const name, const char* const var);

    template<typename T>
    Assert& Variable(const char* const name, T* const var);

    template<typename T>
    Assert& Variable(const char* const name, const T* const var);

    template<typename T, class = typename std::enable_if<std::is_enum<T>::value>::type>
    Assert& Variable(const char* const name, const T var);

    template<typename T, class = typename std::enable_if<!std::is_enum<T>::value>::type>
    Assert& Variable(const char* const name, const T& var);

private:
    // Dispatches the name and value of a generic type to registered assertion handlers and loggers.
    static void Dispatch(const SourceInfo& sourceInfo, const char* const name, const char* format, ...);
    static void DispatchInternal(const SourceInfo& sourceInfo, const char* const name, const char* pValue);

    X_NO_COPY(Assert);
    X_NO_ASSIGN(Assert);

    const SourceInfo& sourceInfo_;
};

#include "Assert.inl"

X_NAMESPACE_END

#endif // X_ASSERT_H_
