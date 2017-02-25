#pragma once


X_NAMESPACE_BEGIN(net)

#define NET_IPv6_SUPPORT 1


// build config specific values
#if X_DEBUG


#elif X_RELEASE


#elif X_SUPER


#else
#error "unkonw config"
#endif


X_NAMESPACE_END