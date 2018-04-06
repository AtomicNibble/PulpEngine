
#pragma once
#ifndef X_NAMESPACEMACROS_H_
#define X_NAMESPACEMACROS_H_

// Namespace it up !

#define X_NAMESPACE_NAME Potato
#define X_NAMESPACE(name) X_NAMESPACE_NAME::name
#define X_NAMESPACE_BEGIN(name)      \
    namespace X_NAMESPACE_NAME::name \
    {
#define X_NAMESPACE_END }
#define X_NAMESPACE_DECLARE(name, symbol) \
    X_NAMESPACE_BEGIN(name)               \
    symbol;                               \
    X_NAMESPACE_END
#define X_USING_NAMESPACE using namespace X_NAMESPACE_NAME

#endif // X_NAMESPACEMACROS_H_
