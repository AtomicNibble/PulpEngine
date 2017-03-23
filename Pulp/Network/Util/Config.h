#pragma once


X_NAMESPACE_BEGIN(net)



// build config specific values
#if X_DEBUG


#elif X_RELEASE


#elif X_SUPER


#else
#error "unkonw config"
#endif


X_NAMESPACE_END