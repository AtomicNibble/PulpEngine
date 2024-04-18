#pragma once


X_NAMESPACE_BEGIN(telemetry)

class TELEM_SRV_EXPORT TraceImport
{
public:
    TraceImport();

    bool ingestTraceFile(core::Path<>& path);

private:

};

X_NAMESPACE_END
