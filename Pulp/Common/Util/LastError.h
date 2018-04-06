
// handling the last error is Windows-specific and not available on other platforms
#if X_PLATFORM_WIN32
#include X_INCLUDE(Util/X_PLATFORM/LastError.h)
#else
#error LastError.h is not available for this platform.
#endif
