

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class SynchronizationPrimitive>
const char* const MultiThreadPolicy<SynchronizationPrimitive>::TYPE_NAME = "MultiThreadPolicy";


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class SynchronizationPrimitive>
void MultiThreadPolicy<SynchronizationPrimitive>::Enter(void)
{
	m_primitive.Enter();
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class SynchronizationPrimitive>
void MultiThreadPolicy<SynchronizationPrimitive>::Leave(void)
{
	m_primitive.Leave();
}
