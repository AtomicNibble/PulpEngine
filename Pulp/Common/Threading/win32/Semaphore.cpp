#include "EngineCommon.h"

#include "Semaphore.h"

X_NAMESPACE_BEGIN(core)

Semaphore::Semaphore(int initialValue, int maximumValue)
{
    sema_ = CreateSemaphore(
        NULL,         // default security attributes
        initialValue, // initial count
        maximumValue, // maximum count
        NULL);        // unnamed semaphore

    if (sema_ == NULL) {
        lastError::Description Dsc;
        X_ERROR("Semaphore", "Failed to create semaphore. Err: %s", lastError::ToString(Dsc));
    }
}

X_NAMESPACE_END