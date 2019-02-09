#include "stdafx.h"
#include "TelemetryLib.h"

#include "Util/PrePro.h"

namespace platform
{
    #ifndef NEAR
    #define NEAR
    #endif

    #ifndef FAR
    #define FAR
    #endif

    #include <WinSock2.h>
    #include <Ws2tcpip.h>

//    X_LINK_LIB("Ws2_32.lib");

} // namespace platform


namespace
{

    struct TraceContext
    {
        void* pScratchBuf;
        size_t bufLen;

        bool isEnabled;
        bool _pad[3];
    };

    TraceContexHandle contextToHandle(TraceContext* pCtx)
    {
        return reinterpret_cast<TraceContexHandle>(pCtx);
    }

    TraceContext* handleToContext(TraceContexHandle handle)
    {
        return reinterpret_cast<TraceContext*>(handle);
    }

    bool isValidContext(TraceContexHandle handle)
    {
        return handle != INVALID_TRACE_CONTEX;
    }

} // namespace



bool ttInit(void)
{
 //   platform::WSADATA winsockInfo;
 //
 //   if (platform::WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0) {
 //       return false;
 //   }

    return true;
}

void ttShutDown(void)
{


}

bool ttInitializeContext(TraceContexHandle& out, void* pBuf, size_t bufLen)
{
    auto contexSize = sizeof(TraceContext);
    if (bufLen < contexSize) {
        return false;
    }

    tt_uint8* pBufU8 = reinterpret_cast<tt_uint8*>(pBuf);

    TraceContext* pCtx = reinterpret_cast<TraceContext*>(pBuf);
    pCtx->pScratchBuf = pBufU8 + contexSize;
    pCtx->bufLen = bufLen - contexSize;
    pCtx->isEnabled = true;


    out = contextToHandle(pCtx);
    return true;
}

void ttShutdownContext(TraceContexHandle ctx)
{
    X_UNUSED(ctx);

}

TtError ttOpen(TraceContexHandle ctx, const char* pAppName, const char* pBuildInfo, const char* pServerAddress,
    ConnectionType conType, tt_uint16 serverPort, tt_int32 timeoutMS)
{
    if (!isValidContext(ctx)) {
        return TtError::InvalidContex;
    }

    if (conType != ConnectionType::ReliableUdp) {
        return TtError::InvalidParam;
    }

    X_UNUSED(pAppName);
    X_UNUSED(pBuildInfo);
    X_UNUSED(timeoutMS);

    // need to open a connection yo.
    // should i reuse all my networking stuff.
    // or just use TCP.
    // think it be good if it's all totaly seperate from engine tho.
    // so lets just write some raw winsock shit.
    X_UNUSED(pServerAddress);
    X_UNUSED(serverPort);



    return TtError::Ok;
}

bool ttClose(TraceContexHandle ctx)
{
    X_UNUSED(ctx);
    return true;
}

bool ttTick(TraceContexHandle ctx)
{
    X_UNUSED(ctx);
    return true;
}

bool ttFlush(TraceContexHandle ctx)
{
    X_UNUSED(ctx);
    return true;
}

void ttUpdateSymbolData(TraceContexHandle ctx)
{
    X_UNUSED(ctx);
}

void ttPause(TraceContexHandle ctx, bool pause)
{
    if (!isValidContext(ctx)) {
        return;
    }

    handleToContext(ctx)->isEnabled = pause;
}

bool ttIsPaused(TraceContexHandle ctx)
{
    if (!isValidContext(ctx)) {
        return true;
    }

    return handleToContext(ctx)->isEnabled;
}