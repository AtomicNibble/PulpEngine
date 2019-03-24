

template<class SynchronizationPrimitive>
const char* const MultiThreadPolicy<SynchronizationPrimitive>::TYPE_NAME = "MultiThreadPolicy";

template<class SynchronizationPrimitive>
MultiThreadPolicy<SynchronizationPrimitive>::MultiThreadPolicy()
{
    // The lock for the base memory allocator is created before core is init.
    if (gEnv && gEnv->ctx != INVALID_TRACE_CONTEX) {
        ttSetLockName(gEnv->ctx, &primitive_, "MemoryLock");
    }
}

template<class SynchronizationPrimitive>
void MultiThreadPolicy<SynchronizationPrimitive>::Enter(void)
{
    primitive_.Enter();
}

template<class SynchronizationPrimitive>
void MultiThreadPolicy<SynchronizationPrimitive>::Leave(void)
{
    primitive_.Leave();
}
