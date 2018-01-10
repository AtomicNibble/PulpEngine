#pragma once

X_NAMESPACE_BEGIN(render)


#if X_DEBUG

#define RENDER_STATS 1
#define VID_MEMORY_STATS 1
#define DEVICE_STATE_STORE_CPU_DESC 1 // for debugging just stores a copy of cpu state used to make device state.
#define PSO_HOT_RELOAD 1 // needed for shader and state hot reloading.

#elif X_RELEASE

#define RENDER_STATS 1
#define VID_MEMORY_STATS 1
#define DEVICE_STATE_STORE_CPU_DESC 0
#define PSO_HOT_RELOAD 0 

#elif X_SUPER

#define RENDER_STATS 0
#define VID_MEMORY_STATS 0
#define DEVICE_STATE_STORE_CPU_DESC 0 
#define PSO_HOT_RELOAD 0 

#else
#error "unkonw config"
#endif


X_NAMESPACE_END