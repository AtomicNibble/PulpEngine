#pragma once


namespace lastErrorWSA
{
    typedef char Description[512];

    TELEMETRY_COMLIB_EXPORT tt_int32 Get(void);

    TELEMETRY_COMLIB_EXPORT TELEM_NO_INLINE const char* ToString(tt_int32 error, Description& desc);
    TELEMETRY_COMLIB_EXPORT TELEM_NO_INLINE const char* ToString(Description& desc);

} // namespace lastErrorWSA
