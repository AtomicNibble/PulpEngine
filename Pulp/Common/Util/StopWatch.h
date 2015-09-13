
#include "System\SystemTimer.h"

X_NAMESPACE_BEGIN(core)

/// \class StopWatch
/// \brief Can be used to measure elpased time since the time was constructed / started
/// Makes use of the game timer providing high-resolution
class xStopWatch
{
public:
	X_INLINE xStopWatch(void);

	X_INLINE void Start(void);

	X_INLINE uint64_t GetCount(void) const;

	X_INLINE float GetSeconds(void) const;
	X_INLINE float GetMilliSeconds(void) const;

private:
	uint64_t start_;
};


#include "StopWatch.inl"

X_NAMESPACE_END

