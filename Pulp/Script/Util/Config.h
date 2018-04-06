#pragma once

X_NAMESPACE_BEGIN(script)

// build config specific values
#if X_DEBUG

#define X_ENABLE_STACK_CHECK 1

#elif X_RELEASE

#define X_ENABLE_STACK_CHECK 1

#elif X_SUPER

#define X_ENABLE_STACK_CHECK 0

#else
#error "unkonw config"
#endif

X_NAMESPACE_END