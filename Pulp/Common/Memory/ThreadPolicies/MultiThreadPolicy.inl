

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<class SynchronizationPrimitive>
const char* const MultiThreadPolicy<SynchronizationPrimitive>::TYPE_NAME = "MultiThreadPolicy";

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<class SynchronizationPrimitive>
void MultiThreadPolicy<SynchronizationPrimitive>::Enter(void)
{
    primitive_.Enter();
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template<class SynchronizationPrimitive>
void MultiThreadPolicy<SynchronizationPrimitive>::Leave(void)
{
    primitive_.Leave();
}
