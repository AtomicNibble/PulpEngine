#pragma once

X_NAMESPACE_BEGIN(render)


#if X_DEBUG


#define RENDER_STATS 1

#elif X_RELEASE

#define RENDER_STATS 1


#elif X_SUPER

#define RENDER_STATS 0


#else
#error "unkonw config"
#endif


X_NAMESPACE_END