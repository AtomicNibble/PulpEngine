#pragma once

X_NAMESPACE_BEGIN(engine)

#if X_DEBUG

#define X_ENABLE_VARIABLE_STATE_STATS 1

#elif X_RELEASE

#define X_ENABLE_VARIABLE_STATE_STATS 0

#elif X_SUPER

#define X_ENABLE_VARIABLE_STATE_STATS 0

#else
#error "unkonw config"
#endif

X_NAMESPACE_END