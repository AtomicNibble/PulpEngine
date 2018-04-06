#pragma once

X_NAMESPACE_BEGIN(net)

// build config specific values
#if X_DEBUG

#define X_ENABLE_NET_STATS 1

#elif X_RELEASE

#define X_ENABLE_NET_STATS 1

#elif X_SUPER

#define X_ENABLE_NET_STATS 0

#else
#error "unkonw config"
#endif

X_NAMESPACE_END