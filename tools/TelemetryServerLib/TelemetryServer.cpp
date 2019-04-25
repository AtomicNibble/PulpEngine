#include "stdafx.h"
#include "TelemetryServer.h"

#include <Time/DateTimeStamp.h>
#include <Util/Guid.h>

#include <IFileSys.h>

#include <../TelemetryCommon/TelemetryCommonLib.h>

#include "PacketTypesViewer.h"

X_LINK_LIB("engine_TelemetryCommonLib.lib");

X_NAMESPACE_BEGIN(telemetry)

namespace
{

    const char* DEFAULT_PORT = "8001";

    bool winSockInit(void)
    {
        platform::WSADATA winsockInfo;

        if (platform::WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0) {
            return false;
        }

        return true;
    }

    void winSockShutDown(void)
    {
        if (platform::WSACleanup() != 0) {
            // rip
            return;
        }
    }

#if 0
    bool readPacket(ClientConnection& client, char* pBuffer, int& bufLengthInOut)
    {
        // this should return complete packets or error.
        int bytesRead = 0;
        int bufLength = sizeof(PacketBase);

        while (1) {
            int maxReadSize = bufLength - bytesRead;
            int res = platform::recv(client.socket, &pBuffer[bytesRead], maxReadSize, 0);

            if (res == 0) {
                X_ERROR("TelemSrv", "Connection closing...");
                return false;
            }
            else if (res < 0) {
                X_ERROR("TelemSrv", "recv failed with error: %d", platform::WSAGetLastError());
                return false;
            }

            bytesRead += res;

            // X_LOG0("TelemSrv", "got: %d bytes", res);

            if (bytesRead == sizeof(PacketBase))
            {
                auto* pHdr = reinterpret_cast<const PacketBase*>(pBuffer);
                if (pHdr->dataSize == 0) {
                    X_ERROR("TelemSrv", "Client sent packet with length zero...");
                    return false;
                }

                if (pHdr->dataSize > bufLengthInOut) {
                    X_ERROR("TelemSrv", "Client sent oversied packet of size %i...", static_cast<int32_t>(pHdr->dataSize));
                    return false;
                }

                bufLength = pHdr->dataSize;
            }

            if (bytesRead == bufLength) {
                bufLengthInOut = bytesRead;
                return true;
            }
            else if (bytesRead > bufLength) {
                X_ERROR("TelemSrv", "Overread packet bytesRead: %d recvbuflen: %d", bytesRead, bufLength);
                return false;
            }
        }
    }
#endif


    template<typename T>
    inline int32_t getPacketSizeIncArgData(T* pPacket)
    {
        return sizeof(T) + pPacket->argDataSize;
    }


    struct IOCPJobData
    {
        HANDLE hIOCP;
    };

    struct ProcessPacketJobData
    {
        uint8_t* pBuf;
        int32_t length;
    };


} // namespace


void ClientConnection::processNetPacketJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pJobData)
{
    X_UNUSED(jobSys, threadIdx, pJob, pJobData);
    auto* pPacketJobData = reinterpret_cast<ProcessPacketJobData*>(pJobData);
    auto* pData = pPacketJobData->pBuf;

    auto* pPacketHdr = reinterpret_cast<const PacketBase*>(pData);

    bool res = false;

    switch (pPacketHdr->type)
    {
        case PacketType::ConnectionRequest:
            res = handleConnectionRequest(pData);
            break;
        case PacketType::ConnectionRequestViewer:
            res = handleConnectionRequestViewer(pData);
            break;
        case PacketType::DataStream:
            res = handleDataSream(pData);
            break;
            // From viewer clients.
        case PacketType::QueryTraceInfo:
            res = handleQueryTraceInfo(pData);
            break;
        case PacketType::OpenTrace:
            res = handleOpenTrace(pData);
            break;
        case PacketType::ReqTraceZoneSegment:
            res = handleReqTraceZoneSegment(pData);
            break;
        case PacketType::ReqTraceLocks:
            res = handleReqTraceLocks(pData);
            break;
        case PacketType::ReqTraceStrings:
            res = handleReqTraceStrings(pData);
            break;
        case PacketType::ReqTraceThreadNames:
            res = handleReqTraceThreadNames(pData);
            break;
        case PacketType::ReqTraceLockNames:
            res = handleReqTraceLockNames(pData);
            break;
        case PacketType::ReqTraceZoneTree:
            res = handleReqTraceZoneTree(pData);
            break;

        default:
            X_ERROR("TelemSrv", "Unknown packet type %" PRIi32, static_cast<int>(pPacketHdr->type));
    }

    if (!res) {
        X_ERROR("TelemSrv", "Error processing packet");
    }
}


bool ClientConnection::handleConnectionRequest(uint8_t* pData)
{
    auto* pConReq = reinterpret_cast<const ConnectionRequestHdr*>(pData);
    if (pConReq->type != PacketType::ConnectionRequest) {
        X_ASSERT_UNREACHABLE();
    }

    VersionInfo serverVer;
    serverVer.major = TELEM_VERSION_MAJOR;
    serverVer.minor = TELEM_VERSION_MINOR;
    serverVer.patch = TELEM_VERSION_PATCH;
    serverVer.build = TELEM_VERSION_BUILD;

    if (pConReq->clientVer != serverVer) {
        sendConnectionRejected("Client server version incompatible");
        return false;
    }
    if (pConReq->appNameLen < 1) {
        sendConnectionRejected("Invalid app name");
        return false;
    }

    clientVer_ = pConReq->clientVer;

    auto* pStrData = reinterpret_cast<const char*>(pConReq + 1);
    auto* pAppNameStr = pStrData;
    auto* pBuildInfoStr = pAppNameStr + pConReq->appNameLen;
    auto* pCmdLineStr = pBuildInfoStr + pConReq->buildInfoLen;

    // Get the app name and see if we have it already.
    TelemFixedStr appName;
    appName.set(pAppNameStr, pAppNameStr + pConReq->appNameLen);

    // Create a new trace 
    Trace trace;
    trace.guid = core::Guid::newGuid();
    trace.buildInfo.assign(pBuildInfoStr, pConReq->buildInfoLen);
    trace.cmdLine.assign(pCmdLineStr, pConReq->cmdLineLen);
    trace.ticksPerMicro = pConReq->ticksPerMicro;
    trace.ticksPerMs = pConReq->ticksPerMs;

    core::Path<> workingDir;
    if (!gEnv->pFileSys->getWorkingDirectory(workingDir)) {
        return false;
    }

    core::DateTimeStamp date = core::DateTimeStamp::getSystemDateTime();
    core::DateTimeStamp::Description dateStr;

    core::Path<> dbPath(workingDir);
    // want a folder for each app.
    dbPath.ensureSlash();
    dbPath.append("traces");
    dbPath.ensureSlash();
    dbPath.append(appName.begin(), appName.end());
    dbPath.ensureSlash();

    if (!gEnv->pFileSys->directoryExists(dbPath, core::VirtualDirectory::BASE)) {
        if (!gEnv->pFileSys->createDirectoryTree(dbPath, core::VirtualDirectory::BASE)) {
            return false;
        }
    }

    // add filename,
    core::Guid::GuidStr guidStr;

    dbPath.append("telem_");
    dbPath.append(date.toString(dateStr));
    dbPath.append("_");
    dbPath.append(trace.guid.toString(guidStr));
    dbPath.setExtension("db");


    trace.dbPath = dbPath;
    trace.hostName.assign(host_.begin(), host_.end());

    // open a trace stream for the conneciton.
    auto& strm = traceBuilder_;
    if (!strm.createDB(dbPath)) {
        return false;
    }

    bool setMeta = true;

    VersionInfo::Description verStr;
    setMeta &= strm.setMeta("guid", trace.guid.toString(guidStr));
    setMeta &= strm.setMeta("appName", core::string_view(appName));
    setMeta &= strm.setMeta("hostName", core::string_view(host_));
    setMeta &= strm.setMeta("buildInfo", trace.buildInfo);
    setMeta &= strm.setMeta("cmdLine", trace.cmdLine);
    setMeta &= strm.setMeta("dateStamp", dateStr);
    setMeta &= strm.setMeta("clientVer", clientVer_.toString(verStr));
    setMeta &= strm.setMeta("serverVer", serverVer.toString(verStr));
    setMeta &= strm.setMeta<int64_t>("tickPerMicro", static_cast<int64_t>(trace.ticksPerMicro));
    setMeta &= strm.setMeta<int64_t>("tickPerMs", static_cast<int64_t>(trace.ticksPerMs));

    if (!setMeta) {
        return false;
    }

    srv_.addTraceForApp(appName, trace);

    // Meow...
    X_LOG0("TelemSrv", "ConnectionAccepted:");
    X_LOG0("TelemSrv", "> AppName: %s", appName.c_str());
    X_LOG0("TelemSrv", "> BuildInfo: %s", trace.buildInfo.c_str());
    X_LOG0("TelemSrv", "> CmdLine: %s", trace.cmdLine.c_str());
    X_LOG0("TelemSrv", "> DB: %s", dbPath.c_str());

    // send a packet back!
    ConnectionRequestAcceptedHdr cra;
    core::zero_object(cra);
    cra.dataSize = sizeof(cra);
    cra.type = PacketType::ConnectionRequestAccepted;
    cra.serverVer = serverVer;

    sendDataToClient(&cra, sizeof(cra));

    X_ASSERT(type_ == ClientType::Unknown, "Client type already set")(type_);
    type_ = ClientType::TraceStream;
    return true;
}



bool ClientConnection::handleConnectionRequestViewer(uint8_t* pData)
{
    auto* pConReq = reinterpret_cast<const ConnectionRequestViewerHdr*>(pData);
    if (pConReq->type != PacketType::ConnectionRequestViewer) {
        X_ASSERT_UNREACHABLE();
    }

    VersionInfo serverVer;
    serverVer.major = TELEM_VERSION_MAJOR;
    serverVer.minor = TELEM_VERSION_MINOR;
    serverVer.patch = TELEM_VERSION_PATCH;
    serverVer.build = TELEM_VERSION_BUILD;

    if (pConReq->viewerVer != serverVer) {
        sendConnectionRejected("Viewer version incompatible with server");
        return false;
    }

    clientVer_ = pConReq->viewerVer;

    // Meow...
    X_LOG0("TelemSrv", "ConnectionAccepted(Viewer):");

    // send a packet back!
    ConnectionRequestAcceptedHdr cra;
    core::zero_object(cra);
    cra.dataSize = sizeof(cra);
    cra.type = PacketType::ConnectionRequestAccepted;
    cra.serverVer = serverVer;

    sendDataToClient(&cra, sizeof(cra));
    
    // send them some data.
    if (!srv_.sendAppList(*this)) {
        X_LOG0("TelemSrv", "Error sending app list to client");
    }

    X_ASSERT(type_ == ClientType::Unknown, "Client type already set")(type_);
    type_ = ClientType::Viewer;
    return true;
}


bool ClientConnection::handleDataSream(uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const DataStreamHdr*>(pData);
    if (pHdr->type != PacketType::DataStream) {
        X_ASSERT_UNREACHABLE();
    }

    // the data is compressed.
    // decompress it..
    int32_t cmpLen = pHdr->dataSize - sizeof(DataStreamHdr);
    int32_t origLen = pHdr->origSize - sizeof(DataStreamHdr);
    X_UNUSED(cmpLen);

    if (cmpLen == origLen) {
        // uncompressed packets.
        X_ASSERT_NOT_IMPLEMENTED();
    }

    auto* pDst = &cmpRingBuf_[cmpBufBegin_];

    int32_t cmpLenOut = static_cast<int32_t>(lz4DecodeStream_.decompressContinue(pHdr + 1, pDst, origLen));
    if (cmpLenOut != cmpLen) {
        // TODO: ..
        return false;
    }

    cmpBufBegin_ += origLen;
    if (cmpBufBegin_ >= (COMPRESSION_RING_BUFFER_SIZE - COMPRESSION_MAX_INPUT_SIZE)) {
        cmpBufBegin_ = 0;
    }

    auto& strm = traceBuilder_;
    sql::SqlLiteTransaction trans(strm.con);

    // process this data?
    for (int32 i = 0; i < origLen; )
    {
        // packet me baby!
        auto* pPacket = reinterpret_cast<const DataPacketBase*>(&pDst[i]);

        switch (pPacket->type)
        {
            case DataStreamType::StringTableAdd:
            {
                i += strm.handleDataPacketStringTableAdd(reinterpret_cast<const DataPacketStringTableAdd*>(&pDst[i]));
                break;
            }
            case DataStreamType::Zone:
            {
                i += strm.handleDataPacketZone(reinterpret_cast<const DataPacketZone*>(&pDst[i]));
                break;
            }
            case DataStreamType::TickInfo:
            {
                i += strm.handleDataPacketTickInfo(reinterpret_cast<const DataPacketTickInfo*>(&pDst[i]));
                break;
            }
            case DataStreamType::ThreadSetName:
            {
                i += strm.handleDataPacketThreadSetName(reinterpret_cast<const DataPacketThreadSetName*>(&pDst[i]));
                break;
            }
            case DataStreamType::CallStack:
            {
                i += strm.handleDataPacketCallStack(reinterpret_cast<const DataPacketCallStack*>(&pDst[i]));
                break;
            }
            case DataStreamType::LockSetName:
            {
                i += strm.handleDataPacketLockSetName(reinterpret_cast<const DataPacketLockSetName*>(&pDst[i]));
                break;
            }
            case DataStreamType::LockTry:
            {
                i += strm.handleDataPacketLockTry(reinterpret_cast<const DataPacketLockTry*>(&pDst[i]));
                break;
            }
            case DataStreamType::LockState:
            {
                i += strm.handleDataPacketLockState(reinterpret_cast<const DataPacketLockState*>(&pDst[i]));
                break;
            }
            case DataStreamType::LockCount:
            {
                i += strm.handleDataPacketLockCount(reinterpret_cast<const DataPacketLockCount*>(&pDst[i]));
                break;
            }
            case DataStreamType::MemAlloc:
            {
                i += strm.handleDataPacketMemAlloc(reinterpret_cast<const DataPacketMemAlloc*>(&pDst[i]));
                break;
            }
            case DataStreamType::MemFree:
            {
                i += strm.handleDataPacketMemFree(reinterpret_cast<const DataPacketMemFree*>(&pDst[i]));
                break;
            }
            case DataStreamType::Message:
            {
                i += strm.handleDataPacketMessage(reinterpret_cast<const DataPacketMessage*>(&pDst[i]));
                break;
            }
            case DataStreamType::Plot:
            {
                i += strm.handleDataPacketPlot(reinterpret_cast<const DataPacketPlot*>(&pDst[i]));
                break;
            }
            case DataStreamType::PDB:
            {
                i += strm.handleDataPacketPDB(reinterpret_cast<const DataPacketPDB*>(&pDst[i]));
                break;
            }
            default:
                X_NO_SWITCH_DEFAULT_ASSERT;
        }
    }

    trans.commit();
    return true;
}


bool ClientConnection::handleQueryTraceInfo(uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const QueryTraceInfo*>(pData);
    if (pHdr->type != PacketType::QueryTraceInfo) {
        X_ASSERT_UNREACHABLE();
    }

    srv_.handleQueryTraceInfo(*this, pHdr);
    return true;
}

bool ClientConnection::handleOpenTrace(uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const OpenTrace*>(pData);
    if (pHdr->type != PacketType::OpenTrace) {
        X_ASSERT_UNREACHABLE();
    }

    core::Guid::GuidStr guidStr;
    X_LOG0("TelemSrv", "Recived trace open request for: \"%s\"", pHdr->guid.toString(guidStr));

    OpenTraceResp otr;
    otr.dataSize = sizeof(otr);
    otr.type = PacketType::OpenTraceResp;
    otr.guid = pHdr->guid;
    otr.ticksPerMicro = 0;
    otr.handle = -1_ui8;

    // TODO: check we don't have it open already.
    // TODO: thread safety etc..
    for (size_t i = 0; i < traces_.size(); i++)
    {
        auto& trace = traces_[i];
        if (trace.trace.guid == pHdr->guid)
        {
            X_WARNING("TelemSrv", "Client opened a trace they already have open");

            if (!TraceDB::getStats(trace.con, otr.stats))
            {
                X_ERROR("TelemSrv", "Failed to get stats for openDb request");
                return true;
            }

            otr.handle = safe_static_cast<int8_t>(i);
            otr.ticksPerMicro = trace.trace.ticksPerMicro;
            sendDataToClient(&otr, sizeof(otr));
            return true;
        }
    }

    // TODO: can't open any more, tell the client?
    if (traces_.size() >= MAX_TRACES_OPEN_PER_CLIENT) {
        return true;
    }

    Trace trace;
    if (!srv_.getTraceForGuid(pHdr->guid, trace)) {
        sendDataToClient(&otr, sizeof(otr));
        return true;
    }

    TraceStream ts;
    // ts.pTrace = &trace;
    if (!ts.openDB(trace.dbPath)) {
        sendDataToClient(&otr, sizeof(otr));
        return true;
    }

    if (!TraceDB::getStats(ts.con, otr.stats))
    {
        X_ERROR("TelemSrv", "Failed to get stats for openDb request");
        return true;
    }

    auto id = traces_.size();
    traces_.emplace_back(std::move(ts));

    otr.handle = safe_static_cast<int8_t>(id);
    otr.ticksPerMicro = trace.ticksPerMicro;
    sendDataToClient(&otr, sizeof(otr));

    return true;
}



X_DISABLE_WARNING(4701) // potentially uninitialized local variable 'tick' used


bool ClientConnection::handleReqTraceZoneSegment(uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceZoneSegment*>(pData);
    if (pHdr->type != PacketType::ReqTraceZoneSegment) {
        X_ASSERT_UNREACHABLE();
    }

    // MEOW
    // load me the ticks!
    int32_t handle = pHdr->handle;
    if (handle < 0 || handle >= static_cast<int32_t>(traces_.size())) {
        return false;
    }

    auto& ts = traces_[pHdr->handle];
    DataPacketTickInfo startTick;
    DataPacketTickInfo endTick;

    {
        auto* pTickHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespTicks>();
        pTickHdr->type = DataStreamTypeViewer::TraceZoneSegmentTicks;
        pTickHdr->num = 0;
        pTickHdr->handle = pHdr->handle;

        auto begin = ts.trace.nanoToTicks(pHdr->startNano);
        auto end = ts.trace.nanoToTicks(pHdr->endNano);

        sql::SqlLiteQuery qry(ts.con, "SELECT threadId, startTick, endTick, startNano, endNano FROM ticks WHERE startTick >= ? AND startTick < ?");
        qry.bind(1, begin);
        qry.bind(2, end);

        auto it = qry.begin();
        if (it == qry.end()) {
            // none
        }

        DataPacketTickInfo tick;
        int32_t numTicks = 0;

        for (; it != qry.end(); ++it) {
            auto row = *it;

            tick.type = DataStreamType::TickInfo;
            tick.threadID = static_cast<uint32_t>(row.get<int32_t>(0));
            tick.start = static_cast<uint64_t>(row.get<int64_t>(1));
            tick.end = static_cast<uint64_t>(row.get<int64_t>(2));
            tick.startNano = static_cast<uint64_t>(row.get<int64_t>(3));
            tick.endNano = static_cast<uint64_t>(row.get<int64_t>(4));

            if (getCompressionBufferSpace() < sizeof(tick))
            {
                // flush etc and add new block header.
                pTickHdr->num = numTicks;
                flushCompressionBuffer();

                pTickHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespTicks>();
                pTickHdr->type = DataStreamTypeViewer::TraceZoneSegmentTicks;
                pTickHdr->num = 0;
                pTickHdr->handle = pHdr->handle;

                numTicks = 0;
            }

            addToCompressionBuffer(&tick, sizeof(tick));

            if (numTicks == 0) {
                startTick = tick;
            }

            ++numTicks;
        }

        if (numTicks == 0)
        {
            X_ASSERT_NOT_IMPLEMENTED();
        }

        // if startTick == endTick it don't matter
        endTick = tick;

        // flush the tick headers.
        pTickHdr->num = numTicks;
        flushCompressionBuffer();
    }


    auto start = startTick.start;
    auto end = endTick.end;

    {

        // so on the client i need to handle overlapping zones.
        // if we just take every zone that starts in the tick and draw it should be okay even if it overlaps.
        // we should still be able to pick it.
        auto* pZonesHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespZones>();
        pZonesHdr->type = DataStreamTypeViewer::TraceZoneSegmentZones;
        pZonesHdr->num = 0;
        pZonesHdr->handle = pHdr->handle;

        int32_t numZones = 0;

        sql::SqlLiteQuery qry(ts.con, "SELECT threadId, startTick, endTick, packedSourceInfo, strIdx FROM zones WHERE startTick >= ? AND startTick < ?");
        qry.bind(1, static_cast<int64_t>(start));
        qry.bind(2, static_cast<int64_t>(end));

        auto it = qry.begin();
        for (; it != qry.end(); ++it) {
            auto row = *it;

            DataPacketZone zone;
            zone.type = DataStreamType::Zone;
            zone.threadID = static_cast<uint32_t>(row.get<int32_t>(0));
            zone.start = static_cast<uint64_t>(row.get<int64_t>(1));
            zone.end = static_cast<uint64_t>(row.get<int64_t>(2));

            TraceBuilder::PackedSourceInfo info;
            info.packed = static_cast<uint64_t>(row.get<int64_t>(3));

            zone.lineNo = info.raw.lineNo;
            zone.strIdxFunction = info.raw.idxFunction;
            zone.strIdxFile = info.raw.idxFile;
            zone.stackDepth = static_cast<uint8_t>(info.raw.depth & 0xFF);

            // TODO: support sending 32bit id to viewer.
            zone.strIdxFmt = static_cast<uint16_t>(row.get<int32_t>(4) & 0xFFFF);

            if (getCompressionBufferSpace() < sizeof(zone))
            {
                // flush etc and add new block header.
                pZonesHdr->num = numZones;
                flushCompressionBuffer();

                pZonesHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespZones>();
                pZonesHdr->type = DataStreamTypeViewer::TraceZoneSegmentZones;
                pZonesHdr->num = 0;
                pZonesHdr->handle = pHdr->handle;

                numZones = 0;
            }

            addToCompressionBuffer(&zone, sizeof(zone));

            ++numZones;
        }

        if (numZones) {
            pZonesHdr->num = numZones;
            flushCompressionBuffer();
        }
    }

    {
        sql::SqlLiteQuery qry(ts.con, "SELECT lockId, threadId, startTick, endTick, result, packedSourceInfo, strIdx FROM lockTry WHERE startTick >= ? AND startTick < ?");
        qry.bind(1, static_cast<int64_t>(start));
        qry.bind(2, static_cast<int64_t>(end));

        auto* pLockTryHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespLockTry>();
        pLockTryHdr->type = DataStreamTypeViewer::TraceZoneSegmentLockTry;
        pLockTryHdr->num = 0;
        pLockTryHdr->handle = pHdr->handle;

        int32_t numLockTry = 0;

        auto it = qry.begin();
        for (; it != qry.end(); ++it) {
            auto row = *it;

            DataPacketLockTry lockTry;
            lockTry.type = DataStreamType::LockTry;
            lockTry.lockHandle = static_cast<uint64_t>(row.get<int64_t>(0));
            lockTry.threadID = static_cast<uint32_t>(row.get<int32_t>(1));
            lockTry.start = static_cast<uint64_t>(row.get<int64_t>(2));
            lockTry.end = static_cast<uint64_t>(row.get<int64_t>(3));
            lockTry.result = static_cast<TtLockResult::Enum>(row.get<int32_t>(4));

            TraceBuilder::PackedSourceInfo info;
            info.packed = static_cast<uint64_t>(row.get<int64_t>(5));

            lockTry.lineNo = info.raw.lineNo;
            lockTry.strIdxFunction = info.raw.idxFunction;
            lockTry.strIdxFile = info.raw.idxFile;
            lockTry.depth = static_cast<uint8_t>(info.raw.depth & 0xFF);

            // TODO: support sending 32bit id to viewer.
            lockTry.strIdxFmt = static_cast<uint16_t>(row.get<int32_t>(6) & 0xFFFF);

            if (getCompressionBufferSpace() < sizeof(lockTry))
            {
                // flush etc and add new block header.
                pLockTryHdr->num = numLockTry;
                flushCompressionBuffer();

                pLockTryHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespLockTry>();
                pLockTryHdr->type = DataStreamTypeViewer::TraceZoneSegmentLockTry;
                pLockTryHdr->num = 0;
                pLockTryHdr->handle = pHdr->handle;

                numLockTry = 0;
            }

            addToCompressionBuffer(&lockTry, sizeof(lockTry));

            ++numLockTry;
        }

        if (numLockTry) {
            pLockTryHdr->num = numLockTry;
            flushCompressionBuffer();
        }
    }

    {
        // lockStates?
        sql::SqlLiteQuery qry(ts.con, "SELECT lockId, threadId, timeTicks, state, packedSourceInfo FROM lockStates WHERE timeTicks >= ? AND timeTicks < ?");
        qry.bind(1, static_cast<int64_t>(start));
        qry.bind(2, static_cast<int64_t>(end));

        auto* pLockStateHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespLockStates>();
        pLockStateHdr->type = DataStreamTypeViewer::TraceZoneSegmentLockStates;
        pLockStateHdr->num = 0;
        pLockStateHdr->handle = pHdr->handle;

        int32_t numLockState = 0;

        auto it = qry.begin();
        for (; it != qry.end(); ++it) {
            auto row = *it;

            DataPacketLockState lockState;
            lockState.type = DataStreamType::LockState;
            lockState.lockHandle = static_cast<uint64_t>(row.get<int64_t>(0));
            lockState.threadID = static_cast<uint32_t>(row.get<int32_t>(1));
            lockState.time = static_cast<uint64_t>(row.get<int64_t>(2));
            lockState.state = static_cast<TtLockState::Enum>(row.get<int32_t>(3));

            TraceBuilder::PackedSourceInfo info;
            info.packed = static_cast<uint64_t>(row.get<int64_t>(4));

            lockState.lineNo = info.raw.lineNo;
            lockState.strIdxFunction = info.raw.idxFunction;
            lockState.strIdxFile = info.raw.idxFile;

            if (getCompressionBufferSpace() < sizeof(lockState))
            {
                // flush etc and add new block header.
                pLockStateHdr->num = numLockState;
                flushCompressionBuffer();

                pLockStateHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespLockStates>();
                pLockStateHdr->type = DataStreamTypeViewer::TraceZoneSegmentLockStates;
                pLockStateHdr->num = 0;
                pLockStateHdr->handle = pHdr->handle;

                numLockState = 0;
            }

            addToCompressionBuffer(&lockState, sizeof(lockState));

            ++numLockState;
        }

        if (numLockState) {
            pLockStateHdr->num = numLockState;
            flushCompressionBuffer();
        }
    }

    X_ASSERT(getCompressionBufferSize() == 0, "Compression buffer is not empty")();
    return true;
}

X_ENABLE_WARNING(4701)


bool ClientConnection::handleReqTraceLocks(uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceLocks*>(pData);
    if (pHdr->type != PacketType::ReqTraceLocks) {
        X_ASSERT_UNREACHABLE();
    }

    int32_t handle = pHdr->handle;
    if (handle < 0 || handle >= static_cast<int32_t>(traces_.size())) {
        return false;
    }

    auto& ts = traces_[pHdr->handle];

    auto* pLocksHdr = addToCompressionBufferT<ReqTraceLocksResp>();
    pLocksHdr->type = DataStreamTypeViewer::TraceLocks;
    pLocksHdr->handle = pHdr->handle;

    int32_t num = 0;

    sql::SqlLiteQuery qry(ts.con, "SELECT id FROM locks");

    auto it = qry.begin();
    for (; it != qry.end(); ++it) {
        auto row = *it;

        TraceLockData tld;
        tld.id = static_cast<uint64_t>(row.get<int64_t>(0));

        if (getCompressionBufferSpace() < sizeof(tld)) {
            pLocksHdr->num = num;
            num = 0;

            flushCompressionBuffer();

            pLocksHdr = addToCompressionBufferT<ReqTraceLocksResp>();
            pLocksHdr->type = DataStreamTypeViewer::TraceLocks;
            pLocksHdr->handle = pHdr->handle;
        }

        addToCompressionBuffer(&tld, sizeof(tld));

        num++;
    }

    if (num) {
        pLocksHdr->num = num;
        flushCompressionBuffer();
    }
    return true;
}

bool ClientConnection::handleReqTraceStrings(uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceStrings*>(pData);
    if (pHdr->type != PacketType::ReqTraceStrings) {
        X_ASSERT_UNREACHABLE();
    }

    int32_t handle = pHdr->handle;
    if (handle < 0 || handle >= static_cast<int32_t>(traces_.size())) {
        return false;
    }

    auto& ts = traces_[pHdr->handle];

    ReqTraceStringsRespInfo info;
    info.type = DataStreamTypeViewer::TraceStringsInfo;
    info.handle = pHdr->handle;

    {
        // lets just count it will be cheap.    
        // TODO: pretty easy to calculate this in ingest if it becomes slow.
        sql::SqlLiteQuery qry(ts.con, "SELECT COUNT(_rowid_), SUM(LENGTH(value)), MIN(Id), MAX(Id) FROM strings");
        auto it = qry.begin();
        if (it == qry.end()) {
            X_ERROR("TelemSrv", "Failed to load string count");
            return false;
        }

        info.num = (*it).get<int32_t>(0);
        info.strDataSize = (*it).get<int32_t>(1);
        info.minId = (*it).get<int32_t>(2);
        info.maxId = (*it).get<int32_t>(3);
    }

    addToCompressionBuffer(&info, sizeof(info));

    // TODO: hack until client can handle multiple per packet
    flushCompressionBuffer();

    auto* pStringsHdr = addToCompressionBufferT<ReqTraceStringsResp>();
    core::zero_this(pStringsHdr);
    pStringsHdr->type = DataStreamTypeViewer::TraceStrings;
    pStringsHdr->handle = pHdr->handle;

    int32_t num = 0;

    if (info.num > 0)
    {
        sql::SqlLiteQuery qry(ts.con, "SELECT id, value FROM strings");

        auto it = qry.begin();
        for (; it != qry.end(); ++it) {
            auto row = *it;

            int32_t id = row.get<int32_t>(0);
            int32_t strLen = row.columnBytes(1);
            const char* pStr = row.get<const char*>(1);

            TraceStringHdr strHdr;
            strHdr.id = static_cast<uint16_t>(id);
            strHdr.length = static_cast<uint16_t>(strLen);

            if (getCompressionBufferSpace() < static_cast<int32_t>(sizeof(strHdr)) + strLen)
            {
                pStringsHdr->num = num;
                num = 0;

                flushCompressionBuffer();

                pStringsHdr = addToCompressionBufferT<ReqTraceStringsResp>();
                core::zero_this(pStringsHdr);
                pStringsHdr->type = DataStreamTypeViewer::TraceStrings;
                pStringsHdr->handle = pHdr->handle;
            }

            addToCompressionBuffer(&strHdr, sizeof(strHdr));
            addToCompressionBuffer(pStr, strLen);

            num++;
        }

        if (num) {
            pStringsHdr->num = num;
            flushCompressionBuffer();
        }
    }

    X_ASSERT(getCompressionBufferSize() == 0, "Compression buffer is not empty")();
    return true;
}

bool ClientConnection::handleReqTraceThreadNames(uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceThreadNames*>(pData);
    if (pHdr->type != PacketType::ReqTraceThreadNames) {
        X_ASSERT_UNREACHABLE();
    }

    int32_t handle = pHdr->handle;
    if (handle < 0 || handle >= static_cast<int32_t>(traces_.size())) {
        return false;
    }

    auto& ts = traces_[pHdr->handle];

    auto* pNamesHdr = addToCompressionBufferT<ReqTraceThreadNamesResp>();
    core::zero_this(pNamesHdr);
    pNamesHdr->type = DataStreamTypeViewer::TraceThreadNames;
    pNamesHdr->handle = pHdr->handle;

    int32_t num = 0;

    sql::SqlLiteQuery qry(ts.con, "SELECT threadId, timeTicks, strIdx FROM threadNames");

    auto it = qry.begin();
    for (; it != qry.end(); ++it) {
        auto row = *it;

        TraceThreadNameData tnd;
        tnd.threadId = row.get<int32_t>(0);
        tnd.timeTicks = row.get<int64_t>(1);
        tnd.strIdx = safe_static_cast<uint16_t>(row.get<int32_t>(2));

        if (getCompressionBufferSpace() < sizeof(tnd)) {
            pNamesHdr->num = num;
            num = 0;

            flushCompressionBuffer();

            pNamesHdr = addToCompressionBufferT<ReqTraceThreadNamesResp>();
            core::zero_this(pNamesHdr);
            pNamesHdr->type = DataStreamTypeViewer::TraceThreadNames;
            pNamesHdr->handle = pHdr->handle;
        }

        addToCompressionBuffer(&tnd, sizeof(tnd));

        num++;
    }

    if (num) {
        pNamesHdr->num = num;
        flushCompressionBuffer();
    }

    return true;
}

bool ClientConnection::handleReqTraceLockNames(uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceLockNames*>(pData);
    if (pHdr->type != PacketType::ReqTraceLockNames) {
        X_ASSERT_UNREACHABLE();
    }

    int32_t handle = pHdr->handle;
    if (handle < 0 || handle >= static_cast<int32_t>(traces_.size())) {
        return false;
    }

    auto& ts = traces_[pHdr->handle];

    auto* pNamesHdr = addToCompressionBufferT<ReqTraceLockNamesResp>();
    core::zero_this(pNamesHdr);
    pNamesHdr->type = DataStreamTypeViewer::TraceLockNames;
    pNamesHdr->handle = pHdr->handle;

    int32_t num = 0;

    sql::SqlLiteQuery qry(ts.con, "SELECT lockId, timeTicks, strIdx FROM lockNames");

    auto it = qry.begin();
    for (; it != qry.end(); ++it) {
        auto row = *it;

        TraceLockNameData lnd;
        lnd.lockId = static_cast<uint64_t>(row.get<int64_t>(0));
        lnd.timeTicks = row.get<int64_t>(1);
        lnd.strIdx = safe_static_cast<uint16_t>(row.get<int32_t>(2));

        if (getCompressionBufferSpace() < sizeof(lnd)) {
            pNamesHdr->num = num;
            num = 0;

            flushCompressionBuffer();

            pNamesHdr = addToCompressionBufferT<ReqTraceLockNamesResp>();
            core::zero_this(pNamesHdr);
            pNamesHdr->type = DataStreamTypeViewer::TraceLockNames;
            pNamesHdr->handle = pHdr->handle;
        }

        addToCompressionBuffer( &lnd, sizeof(lnd));

        num++;
    }

    if (num) {
        pNamesHdr->num = num;
        flushCompressionBuffer();
    }

    return true;
}


bool ClientConnection::handleReqTraceZoneTree(uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceZoneTree*>(pData);
    if (pHdr->type != PacketType::ReqTraceZoneTree) {
        X_ASSERT_UNREACHABLE();
    }

    int32_t handle = pHdr->handle;
    if (handle < 0 || handle >= static_cast<int32_t>(traces_.size())) {
        return false;
    }

    auto& ts = traces_[pHdr->handle];

    auto* pTreeHdr = addToCompressionBufferT<ReqTraceZoneTreeResp>();
    core::zero_this(pTreeHdr);
    pTreeHdr->type = DataStreamTypeViewer::TraceZoneTree;
    pTreeHdr->handle = pHdr->handle;

    int32_t num = 0;

    sql::SqlLiteQuery qry(ts.con, "SELECT parentId, totalTick, strIdx FROM zoneNodes WHERE setId = ?");
    qry.bind(1, pHdr->frameIdx);

    auto it = qry.begin();
    for (; it != qry.end(); ++it) {
        auto row = *it;

        TraceZoneTreeData ztd;
        ztd.parentId = row.get<int32_t>(0);
        ztd.totalTicks = row.get<int64_t>(1);
        ztd.strIdx = safe_static_cast<uint32_t>(row.get<int32_t>(2));

        if (getCompressionBufferSpace() < sizeof(ztd)) {
            pTreeHdr->num = num;
            num = 0;

            flushCompressionBuffer();

            pTreeHdr = addToCompressionBufferT<ReqTraceZoneTreeResp>();
            core::zero_this(pTreeHdr);
            pTreeHdr->type = DataStreamTypeViewer::TraceZoneTree;
            pTreeHdr->handle = pHdr->handle;
        }

        addToCompressionBuffer(&ztd, sizeof(ztd));

        num++;
    }

    if (num) {
        pTreeHdr->num = num;
        flushCompressionBuffer();
    }

    return true;
}

void ClientConnection::sendDataToClient(const void* pData, size_t len)
{
    // send some data...
    int res = platform::send(socket_, reinterpret_cast<const char*>(pData), static_cast<int>(len), 0);
    if (res == SOCKET_ERROR) {
        X_LOG0("TelemSrv", "send failed with error: %d", platform::WSAGetLastError());
    }
}

void ClientConnection::sendConnectionRejected(const char* pReason)
{
    X_LOG0("TelemSrv", "ConnectionRejected:");

    size_t msgLen = strlen(pReason);
    size_t datalen = sizeof(ConnectionRequestRejectedHdr) + msgLen;

    if (msgLen > MAX_STRING_LEN) {
        msgLen = MAX_STRING_LEN;
    }

    char buf[sizeof(ConnectionRequestRejectedHdr) + MAX_STRING_LEN];

    auto* pCr = reinterpret_cast<ConnectionRequestRejectedHdr*>(buf);
    pCr->dataSize = static_cast<tt_uint16>(datalen);
    pCr->type = PacketType::ConnectionRequestRejected;

    memcpy(pCr + 1, pReason, msgLen);
    sendDataToClient(buf, datalen);
}


void ClientConnection::flushCompressionBuffer(void)
{
    // compress it.
    const auto* pInBegin = &cmpRingBuf_[cmpBufBegin_];
    const size_t inBytes = cmpBufEnd_ - cmpBufBegin_;

#if X_DEBUG
    if (inBytes > COMPRESSION_MAX_INPUT_SIZE) {
        ::DebugBreak();
    }
#endif // X_DEBUG

    if (inBytes == 0) {
        return;
    }

    constexpr size_t cmpBufSize = core::Compression::LZ4Stream::requiredDeflateDestBuf(COMPRESSION_MAX_INPUT_SIZE);
    char cmpBuf[cmpBufSize + sizeof(DataStreamHdr)];

    const size_t cmpBytes = lz4Stream_.compressContinue(
        pInBegin, inBytes,
        cmpBuf + sizeof(DataStreamHdr), cmpBufSize,
        core::Compression::CompressLevel::NORMAL
    );

    if (cmpBytes <= 0) {
        // TODO: error.
    }

    const size_t totalLen = cmpBytes + sizeof(DataStreamHdr);

    DataStreamHdr* pHdr = reinterpret_cast<DataStreamHdr*>(cmpBuf);

    // patch the length 
    pHdr->type = PacketType::DataStream;
    pHdr->dataSize = static_cast<tt_uint16>(totalLen);
    pHdr->origSize = static_cast<tt_uint16>(inBytes + sizeof(DataStreamHdr));

    sendDataToClient(cmpBuf, totalLen);

    cmpBufBegin_ = cmpBufEnd_;
    if ((sizeof(cmpRingBuf_) - cmpBufBegin_) < COMPRESSION_MAX_INPUT_SIZE) {
        cmpBufBegin_ = 0;
        cmpBufEnd_ = 0;
    }
}

X_INLINE int32_t ClientConnection::getCompressionBufferSize(void) const
{
    return cmpBufEnd_ - cmpBufBegin_;
}

X_INLINE int32_t ClientConnection::getCompressionBufferSpace(void) const
{
    const int32_t space = COMPRESSION_MAX_INPUT_SIZE - (cmpBufEnd_ - cmpBufBegin_);

    return space;
}

void ClientConnection::addToCompressionBuffer(const void* pData, int32_t len)
{
#if X_DEBUG
    if (len > COMPRESSION_MAX_INPUT_SIZE) {
        ::DebugBreak();
    }
#endif // X_DEBUG

    // can we fit this data?
    const int32_t space = getCompressionBufferSpace();
    if (space < len) {
        flushCompressionBuffer();
    }

    memcpy(&cmpRingBuf_[cmpBufEnd_], pData, len);
    cmpBufEnd_ += len;
}

template<typename T>
T* ClientConnection::addToCompressionBufferT(void)
{
#if X_DEBUG
    if constexpr (sizeof(T) > COMPRESSION_MAX_INPUT_SIZE) {
        ::DebugBreak();
    }
#endif // X_DEBUG

    // can we fit this data?
    const int32_t space = getCompressionBufferSpace();
    if (space < sizeof(T)) {
        flushCompressionBuffer();
    }

    static_assert(std::is_trivially_copyable_v<T>, "T is not trivially copyable");

    T* pPtr = reinterpret_cast<T*>(&cmpRingBuf_[cmpBufEnd_]);
    cmpBufEnd_ += sizeof(T);
    return pPtr;
}



// ----------------------------------------------

ZoneTree::ZoneTree() :
    root_(core::string_view("root")),
    heap_(
        core::bitUtil::RoundUpToMultiple<size_t>(sizeof(Node) * MAX_NODES,
        core::VirtualMem::GetPageSize())
    ),
    allocator_(heap_.start(), heap_.end()),
    arena_(&allocator_, "ZoneNodeAllocator")
{
}

ZoneTree::~ZoneTree()
{
    free_r(&root_);
}

void ZoneTree::addZone(const StringBuf& buf, const DataPacketZone* pData)
{
    const auto timeTicks = pData->end - pData->start;

    // always add time to root?
    auto* pNode = &root_;
    pNode->info.totalTicks += timeTicks; // add time to root.

    if (buf.isEmpty() || buf[0] != '(') {
        return;
    }

    // need to find either / or )
    const char* pBegin = buf.begin() + 1;
    const char* pCur = pBegin;

    while (1)
    {
        while (pCur < buf.end() && *pCur != '/' && *pCur != ')') {
            ++pCur;
        }

        if (pCur == buf.end()) {
            X_ERROR("TelemSrv", "Unexpected end of path \"%s\"", buf.c_str());
            return;
        }

        core::string_view path(pBegin, pCur);

        // add it as a child.
        if (!pNode->pFirstChild)
        {
            pNode->pFirstChild = X_NEW(Node, &arena_, "PathTreeNode")(path);
            pNode = pNode->pFirstChild;
        }
        else
        {
            pNode = pNode->pFirstChild;

            while (1)
            {
                if (pNode->name.compare(path.begin(), path.length())) {
                    break;
                }

                if (!pNode->pNextsibling) {
                    pNode->pNextsibling = X_NEW(Node, &arena_, "PathTreeNode")(path);
                    pNode = pNode->pNextsibling;
                    break;
                }

                pNode = pNode->pNextsibling;
            }
        }

        X_ASSERT(pNode && pNode->name.compare(path.begin(), path.length()), "Incorrect node")();

        // add time all the way down the tree, so we get aggregation.
        pNode->info.totalTicks += timeTicks;

        if (*pCur == ')') {
            break;
        }

        ++pCur;
        pBegin = pCur;
    }

    // TODO: add leafs?
    // now sure could be a lot of them..
}

void ZoneTree::getNodes(NodeFlatArr& arr) const
{
    arr.clear();

    addNodes_r(arr, -1, &root_);
}

void ZoneTree::addNodes_r(NodeFlatArr& arr, int32_t parIdx, const Node* pNode) const
{
    if (!pNode) {
        return;
    }

    auto idx = static_cast<int32_t>(arr.size());
    {
        auto& node = arr.AddOne();
        node.parentIdx = parIdx;
        node.info = pNode->info;
        node.name = pNode->name;
    }

    if (pNode->pFirstChild) {
        addNodes_r(arr, idx, pNode->pFirstChild);
    }

    pNode = pNode->pNextsibling;
    while (pNode) {

        idx = static_cast<int32_t>(arr.size());
        auto& node = arr.AddOne();
        node.parentIdx = parIdx;
        node.info = pNode->info;
        node.name = pNode->name;

        if (pNode->pFirstChild) {
            addNodes_r(arr, idx, pNode->pFirstChild);
        }

        pNode = pNode->pNextsibling;
    }
}

void ZoneTree::print(void) const
{
    print_r(core::string(""), &root_);
}

void ZoneTree::print_r(const core::string& prefix, const Node* pNode) const
{
    if (!pNode) {
        return;
    }

    core::StackString<128, char> buf;
    buf.set(prefix.begin(), prefix.length());
    buf.append("-");
    buf.append(pNode->name.begin(), pNode->name.length());
    buf.appendFmt(" %" PRIu64, pNode->info.totalTicks);

    X_LOG0("zoneTree", buf.c_str());

    const auto subPrefix = prefix + "|   ";

    if (pNode->pFirstChild) {
        print_r(subPrefix, pNode->pFirstChild);
    }

    pNode = pNode->pNextsibling;
    while (pNode) {

        buf.set(prefix.begin(), prefix.length());
        buf.append("-");
        buf.append(pNode->name.begin(), pNode->name.length());
        buf.appendFmt(" %" PRIu64, pNode->info.totalTicks);

        X_LOG0("zoneTree", buf.c_str());

        if (pNode->pFirstChild) {
            print_r(subPrefix, pNode->pFirstChild);
        }

        pNode = pNode->pNextsibling;
    }
}

void ZoneTree::free_r(const Node* pNode)
{
    if (!pNode) {
        return;
    }

    if (pNode->pFirstChild) {
        free_r(pNode->pFirstChild);
    }

    pNode = pNode->pNextsibling;
    while (pNode) {
        free_r(pNode);
        pNode = pNode->pNextsibling;
    }

    // This is a no-op when we using the LinearAllocator
    X_DELETE(pNode, &arena_);
}

// -----------------------------------------------

bool TraceDB::openDB(core::Path<char>& path)
{
    // TODO: drop write when not needed
    if (!con.connect(path.c_str(), sql::OpenFlag::WRITE)) {
        return false;
    }

    return true;
}

bool TraceDB::setPragmas(void)
{
    return con.execute(R"(
        PRAGMA synchronous = OFF;
        PRAGMA page_size = 4096;
        PRAGMA cache_size = -4000;
        PRAGMA journal_mode = MEMORY;
        PRAGMA foreign_keys = ON;
    )");
}

bool TraceDB::getStats(sql::SqlLiteDb& db, TraceStats& stats)
{
    // These need to be fast even when there is 10 million rows etc..

    {
        sql::SqlLiteQuery qry(db, "SELECT MAX(_rowid_) FROM strings LIMIT 1");
        auto it = qry.begin();
        if (it == qry.end()) {
            X_ERROR("TelemSrv", "Failed to load string count");
            return false;
        }

        stats.numStrings = (*it).get<int64_t>(0);
    }

    {
        sql::SqlLiteQuery qry(db, "SELECT MAX(_rowid_) FROM zones LIMIT 1");
        auto it = qry.begin();
        if (it == qry.end()) {
            X_ERROR("TelemSrv", "Failed to load zone count");
            return false;
        }

        stats.numZones = (*it).get<int64_t>(0);
    }

    {
        sql::SqlLiteQuery qry(db, "SELECT MAX(_rowid_) FROM ticks LIMIT 1");
        auto it = qry.begin();
        if (it == qry.end()) {
            X_ERROR("TelemSrv", "Failed to load tick count");
            return false;
        }

        stats.numTicks = (*it).get<int64_t>(0);
    }

    {
        sql::SqlLiteQuery qry(db, "SELECT MAX(_rowid_) FROM lockTry LIMIT 1");
        auto it = qry.begin();
        if (it == qry.end()) {
            X_ERROR("TelemSrv", "Failed to load lock try count");
            return false;
        }

        stats.numLockTry = (*it).get<int64_t>(0);
    }

    {
        sql::SqlLiteQuery qry(db, "SELECT endNano FROM ticks WHERE _rowid_ = (SELECT MAX(_rowid_) FROM ticks)");
        auto it = qry.begin();
        if (it == qry.end()) {
            X_ERROR("TelemSrv", "Failed to load last tick");
            return false;
        }

        stats.durationNano = (*it).get<int64_t>(0);
    }


    {
        // simular performance.
        // SELECT * FROM zones WHERE _rowid_ = (SELECT MAX(_rowid_) FROM zones);
        // SELECT * FROM zones ORDER BY Id DESC LIMIT 1;
        //sql::SqlLiteQuery qry(db, "SELECT * FROM zones WHERE _rowid_ = (SELECT MAX(_rowid_) FROM zones)");

    }

    return true;
}

bool TraceDB::getMetaStr(sql::SqlLiteDb& db, const char* pName, core::string& strOut)
{
    sql::SqlLiteQuery qry(db, "SELECT value FROM meta WHERE name = ?");
    qry.bind(1, pName);

    auto it = qry.begin();
    if (it == qry.end()) {
        X_ERROR("TelemSrv", "Failed to load meta entry \"%s\"", pName);
        return false;
    }

    auto row = *it;

    auto* pStr = row.get<const char*>(0);
    auto strLen = row.columnBytes(0);

    strOut.assign(pStr, strLen);
    return true;
}

bool TraceDB::getMetaUInt64(sql::SqlLiteDb& db, const char* pName, uint64_t& valOut)
{
    sql::SqlLiteQuery qry(db, "SELECT value FROM meta WHERE name = ?");
    qry.bind(1, pName);

    auto it = qry.begin();
    if (it == qry.end()) {
        X_ERROR("TelemSrv", "Failed to load meta entry \"%s\"", pName);
        return false;
    }

    auto row = *it;

    valOut = static_cast<uint64_t>(row.get<int64_t>(0));
    return true;
}

// -----------------------------------------------

bool TraceBuilder::createDB(core::Path<char>& path)
{
    if (!con.connect(path.c_str(), sql::OpenFlag::CREATE | sql::OpenFlag::WRITE)) {
        return false;
    }

    if (!setPragmas()) {
        return false;
    }

    // create all the tables.
    if (!createTables()) {
        return false;
    }

    bool okay = true;

    okay &= (sql::Result::OK == cmdInsertZone.prepare("INSERT INTO zones (threadID, startTick, endTick, packedSourceInfo, strIdx) VALUES(?,?,?,?,?)"));
    okay &= (sql::Result::OK == cmdInsertString.prepare("INSERT INTO strings (Id, value) VALUES(?, ?)"));
    okay &= (sql::Result::OK == cmdInsertTickInfo.prepare("INSERT INTO ticks (threadId, startTick, endTick, startNano, endNano) VALUES(?,?,?,?,?)"));
    okay &= (sql::Result::OK == cmdInsertLock.prepare("INSERT INTO locks (Id) VALUES(?)"));
    okay &= (sql::Result::OK == cmdInsertLockTry.prepare("INSERT INTO lockTry (lockId, threadId, startTick, endTick, result, packedSourceInfo, strIdx) VALUES(?,?,?,?,?,?,?)"));
    okay &= (sql::Result::OK == cmdInsertLockState.prepare("INSERT INTO lockStates (lockId, threadId, timeTicks, state, packedSourceInfo, strIdx) VALUES(?,?,?,?,?,?)"));
    okay &= (sql::Result::OK == cmdInsertLockName.prepare("INSERT INTO lockNames (lockId, timeTicks, strIdx) VALUES(?,?,?)"));
    okay &= (sql::Result::OK == cmdInsertThreadName.prepare("INSERT INTO threadNames (threadId, timeTicks, strIdx) VALUES(?,?,?)"));
    okay &= (sql::Result::OK == cmdInsertMeta.prepare("INSERT INTO meta (name, value) VALUES(?,?)"));
    okay &= (sql::Result::OK == cmdInsertMemory.prepare("INSERT INTO memory (allocId, size, threadId, timeTicks, operation, packedSourceInfo, strIdx) VALUES(?,?,?,?,?,?,?)"));
    okay &= (sql::Result::OK == cmdInsertMessage.prepare("INSERT INTO messages (timeTicks, type, strIdx) VALUES(?,?,?)"));
    okay &= (sql::Result::OK == cmdInsertZoneNode.prepare("INSERT INTO zoneNodes (setId, parentId, totalTick, count, strIdx) VALUES(?,?,?,?,?)"));
    okay &= (sql::Result::OK == cmdInsertPlot.prepare("INSERT INTO plots (type, timeTicks, value, strIdx) VALUES(?,?,?,?)"));
    okay &= (sql::Result::OK == cmdInsertPDB.prepare("INSERT INTO sym (modAddr, imageSize, guid, strIdx) VALUES(?,?,?,?)"));
    okay &= (sql::Result::OK == cmdInsertCallstack.prepare("INSERT INTO callstack (id, depth, data) VALUES(?,?,?)"));

    return okay;
}

bool TraceBuilder::createIndexes(void)
{
    sql::SqlLiteTransaction trans(con);

    sql::SqlLiteCmd cmd(con, R"(
        CREATE INDEX IF NOT EXISTS "zones_start" ON "zones" (
            "startTick"	ASC
        );
        CREATE INDEX IF NOT EXISTS "ticks_start" ON "ticks" (
            "startTick"	ASC
        );
        CREATE INDEX IF NOT EXISTS "lockTry_start" ON "lockTry" (
            "startTick"	ASC
        );
        CREATE INDEX IF NOT EXISTS "lockStates_time" ON "lockStates" (
            "timeTicks"	ASC
        );
        CREATE INDEX IF NOT EXISTS "messages_time" ON "messages" (
            "timeTicks"	ASC
        );
        CREATE INDEX IF NOT EXISTS "zoneNode_setid" ON "zoneNodes" (
            "setId"	ASC
        );
        CREATE INDEX IF NOT EXISTS "callstack_id" ON "callstack" (
            "id"	ASC
        );
    )");

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "Failed to creat indexes");
        return false;
    }

    trans.commit();
    return true;
}

bool TraceBuilder::createTables(void)
{
    if (!con.execute(R"(

CREATE TABLE IF NOT EXISTS "meta" (
	"Id"	            INTEGER,
	"name"	            TEXT NOT NULL UNIQUE,
	"value"	            TEXT NOT NULL,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "ticks" (
	"Id"	            INTEGER,
	"threadId"	        INTEGER NOT NULL,
	"startTick"	        INTEGER NOT NULL,
	"endTick"           INTEGER NOT NULL,
	"startNano"	        INTEGER NOT NULL,
	"endNano"	        INTEGER NOT NULL,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "threadNames" (
    "Id"                INTEGER,
    "threadId"          INTEGER NOT NULL,
    "timeTicks"         INTEGER NOT NULL,
    "strIdx"            INTEGER NOT NULL,
    PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "strings" (
	"Id"	            INTEGER,
	"value"	            TEXT NOT NULL UNIQUE,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "zones" (
    "id"	            INTEGER,
    "threadId"	        INTEGER NOT NULL,
    "startTick"	        INTEGER NOT NULL,
    "endTick"	        INTEGER NOT NULL,
    "packedSourceInfo"	INTEGER NOT NULL,
    "strIdx"	        INTEGER NOT NULL,
    PRIMARY KEY("id")
);

CREATE TABLE IF NOT EXISTS "zoneNodes" (
    "id"	            INTEGER,
    "setId"	            INTEGER NOT NULL,
    "parentId"	        INTEGER NOT NULL,
    "totalTick"	        INTEGER NOT NULL,
    "count"	            INTEGER NOT NULL,
    "strIdx"	        INTEGER NOT NULL,
    PRIMARY KEY("id")
);

CREATE TABLE IF NOT EXISTS "locks" (
	"Id"	            INTEGER,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "lockNames" (
    "Id"                INTEGER,
    "lockId"            INTEGER NOT NULL,
    "timeTicks"         INTEGER NOT NULL,
    "strIdx"            INTEGER NOT NULL,
    PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "lockTry" (
	"Id"	            INTEGER,
	"lockId"	        INTEGER NOT NULL,
    "threadId"	        INTEGER NOT NULL,
	"startTick"	        INTEGER NOT NULL,
	"endTick"	        INTEGER NOT NULL,
    "result"	        INTEGER NOT NULL,
	"packedSourceInfo"	INTEGER NOT NULL,
	"strIdx"	        INTEGER NOT NULL,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "lockStates" (
	"Id"	            INTEGER,
	"lockId"	        INTEGER NOT NULL,
	"threadId"	        INTEGER NOT NULL,
	"timeTicks"	        INTEGER NOT NULL,
	"state"	            INTEGER NOT NULL,
    "packedSourceInfo"	INTEGER NOT NULL,
	"strIdx"	        INTEGER NOT NULL,
	PRIMARY KEY("Id")
);

CREATE TABLE "memory" (
	"Id"	            INTEGER,
	"allocId"	        INTEGER NOT NULL,
	"size"	            INTEGER NOT NULL,
	"threadId"	        INTEGER NOT NULL,
	"timeTicks"	        INTEGER NOT NULL,
	"operation"	        INTEGER NOT NULL,
    "packedSourceInfo"	INTEGER NOT NULL,
	"strIdx"	        INTEGER,
	PRIMARY KEY("Id")
);

CREATE TABLE "messages" (
	"Id"	            INTEGER,
	"type"	            INTEGER NOT NULL,
	"timeTicks"	        INTEGER NOT NULL,
	"strIdx"	        INTEGER NOT NULL,
	PRIMARY KEY("Id")
);

CREATE TABLE "plots" (
	"Id"	            INTEGER,
	"type"	            INTEGER NOT NULL,
	"timeTicks"	        INTEGER NOT NULL,
	"value"	            BLOB NOT NULL,
	"strIdx"	        INTEGER NOT NULL,
	PRIMARY KEY("Id")
);

CREATE TABLE "sym" (
    "Id"                INTEGER,
    "modAddr"           INTEGER NOT NULL,
    "imageSize"         INTEGER NOT NULL,
    "guid"              BLOB NOT NULL,
    "strIdx"            INTEGER NOT NULL,
    PRIMARY KEY("Id")
);

CREATE TABLE "callstack" (
    "Id"                INTEGER,
    "depth"             INTEGER NOT NULL,
    "data"             BLOB NOT NULL,
    PRIMARY KEY("Id")
);

            )")) {
        X_ERROR("TelemSrv", "Failed to create tables");
        return false;
    }

    return true;
}

template<typename T>
bool TraceBuilder::setMeta(const char* pName, T value)
{
    auto& cmd = cmdInsertMeta;
    cmd.reset();
    cmd.bind(1, pName);
    cmd.bind(2, value);

    sql::Result::Enum res = cmd.execute();
    if (res != sql::Result::OK) {
        return false;
    }

    return true;
}

void TraceBuilder::insertLockIfMissing(uint64_t lockHandle)
{
    // look up the lock.
    if (std::binary_search(lockSet.begin(), lockSet.end(), lockHandle)) {
        return;
    }

    lockSet.push_back(lockHandle);
    std::sort(lockSet.begin(), lockSet.end());

    auto& cmd = cmdInsertLock;
    cmd.bind(1, static_cast<int64_t>(lockHandle));
    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
}

int32_t TraceBuilder::addString(core::string_view str)
{
    auto it = stringMap.emplace(core::string(str.begin(), str.end()), 0);
    X_ASSERT(it.second, "Duplicate")();
    auto idx = static_cast<int32_t>(it.first.getIndex());

    auto& cmd = cmdInsertString;
    cmd.bind(1, idx);
    cmd.bind(2, str.data(), str.length());

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return static_cast<int32_t>(idx);
}

int32_t TraceBuilder::indexForString(core::string_view str)
{
    auto it = stringMap.find(str);
    if (it != stringMap.end()) {
        return static_cast<int32_t>(it.getIndex());
    }

    return addString(str);
}

uint16_t TraceBuilder::getStringIndex(uint16_t strIdx) const
{
    auto idx = indexMap[strIdx].idx;
    X_ASSERT(idx != std::numeric_limits<uint16_t>::max(), "Index not valid")(strIdx, idx);
    return idx;
}

uint16_t TraceBuilder::getStringIndex(StringBuf& buf, const DataPacketBaseArgData* pPacket, int32_t packetSize, uint16_t strIdxFmt)
{
    int32_t strIdx = -1;

    if (pPacket->argDataSize)
    {
        auto* pArgDataBytes = reinterpret_cast<const uint8_t*>(pPacket) + packetSize;
        auto* pArgData = reinterpret_cast<const ArgData*>(pArgDataBytes);

        // i need the format string.
        // don't really have them to hand.
        auto fmtStrIdx = indexMap[strIdxFmt].idx;
        auto fmtIt = stringMap.at(fmtStrIdx);
        auto& fmtStr = fmtIt->first;

        sprintf_ArgData(buf, fmtStr.c_str(), *pArgData);

        core::string_view view(buf.data(), buf.length());

        // now we need to check if this string is unique.
        auto it = stringMap.find(view);
        if (it == stringMap.end()) {
            strIdx = addString(view);
        }
        else {
            strIdx = static_cast<int32_t>(it.getIndex());
        }
    }
    else {

        // map it to string in the dynamic table.
        auto str = indexMap[strIdxFmt];

        strIdx = str.idx;

        // we can lookup the string.
        // think ideally this should just return a view maybe?
        // currently i'm going to have to copy the string each call :(
        buf.set(str.str.data(), str.str.length());
    }

    X_ASSERT(strIdx != -1, "Failed to get index")(strIdx, pPacket->argDataSize);
    return safe_static_cast<uint16_t>(strIdx);
}

int32_t TraceBuilder::handleDataPacketTickInfo(const DataPacketTickInfo* pData)
{
    auto& cmd = cmdInsertTickInfo;
    cmd.bind(1, static_cast<int32_t>(pData->threadID));
    cmd.bind(2, static_cast<int64_t>(pData->start));
    cmd.bind(3, static_cast<int64_t>(pData->end));
    cmd.bind(4, static_cast<int64_t>(pData->startNano));
    cmd.bind(5, static_cast<int64_t>(pData->endNano));

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return sizeof(std::remove_pointer_t<decltype(pData)>);
}

int32_t TraceBuilder::handleDataPacketStringTableAdd(const DataPacketStringTableAdd* pData)
{
    const int32_t packetSize = sizeof(std::remove_pointer_t<decltype(pData)>) + pData->length;
    const char* pString = reinterpret_cast<const char*>(pData + 1);
    
    core::string_view str(pString, pData->length);

    // Ah we have a problem in that if you don't have string pooling on.
    // we end up with matching strings here that have diffrent pData->id.
    // so we just need to make the other instances point at first one

    auto it = stringMap.find(str);
    if (it == stringMap.end()) {
        auto idx = addString(str);
        indexMap[pData->id] = { safe_static_cast<uint16_t>(idx), core::string(str.begin(), str.length()) };
    }
    else if(indexMap[pData->id].idx == std::numeric_limits<uint16_t>::max()) {
        auto idx = safe_static_cast<uint16_t>(it.getIndex());
        indexMap[pData->id] = { idx, core::string(str.begin(), str.length()) };

        // log warning.
        X_WARNING_EVERY_N(10, "TelemSrv", "Recived duplicate string \"%.*s\" check string pooling is on", str.length(), str.data());
    }

    return packetSize;
}

inline void TraceBuilder::accumulateZoneData(const StringBuf& buf, int32_t strIdx, const DataPacketZone* pData)
{
    X_UNUSED(strIdx);

    // so we need like a FIFO queue
    // and we dump zones for that frame into it.
    // problem is need to handle overlap?
    // fuck.
    // need to think some more...

    zoneTree_.addZone(buf, pData);
}

void TraceBuilder::flushZoneTree(void)
{

    writeZoneTree(zoneTree_, -1);
}

void TraceBuilder::writeZoneTree(const ZoneTree& zoneTree, int32_t setID)
{
    ZoneTree::NodeFlatArr arr(g_TelemSrvLibArena);
    zoneTree.getNodes(arr);

    if (!arr.isNotEmpty()) {
        return;
    }

    for (auto& node : arr)
    {
        auto strIdx = indexForString(core::string_view(node.name));
    
        auto& cmd = cmdInsertZoneNode;
        cmd.bind(1, setID);
        cmd.bind(2, node.parentIdx);
        cmd.bind(3, static_cast<int64_t>(node.info.totalTicks));
        cmd.bind(4, static_cast<int64_t>(node.info.count));
        cmd.bind(5, strIdx);
    
        auto res = cmd.execute();
        if (res != sql::Result::OK) {
            X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
        }
    
        cmdInsertZoneNode.reset();
    }
}


int32_t TraceBuilder::handleDataPacketZone(const DataPacketZone* pData)
{
    StringBuf strBuf;
    int32_t strIdx = getStringIndex(strBuf, pData, sizeof(*pData), pData->strIdxFmt);

    PackedSourceInfo info;
    info.raw.lineNo = pData->lineNo;
    info.raw.idxFunction = getStringIndex(pData->strIdxFunction);
    info.raw.idxFile = getStringIndex(pData->strIdxFile);
    info.raw.depth = pData->stackDepth;

    auto& cmd = cmdInsertZone;
    cmd.bind(1, static_cast<int32_t>(pData->threadID));
    cmd.bind(2, static_cast<int64_t>(pData->start));
    cmd.bind(3, static_cast<int64_t>(pData->end));
    cmd.bind(4, static_cast<int64_t>(info.packed));
    cmd.bind(5, strIdx);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();

    accumulateZoneData(strBuf, strIdx, pData);

    return sizeof(*pData) + pData->argDataSize;
}

int32_t TraceBuilder::handleDataPacketLockTry(const DataPacketLockTry* pData)
{
    insertLockIfMissing(pData->lockHandle);

    StringBuf strBuf;
    int32_t strIdx = getStringIndex(strBuf, pData, sizeof(*pData), pData->strIdxFmt);

    PackedSourceInfo info;
    info.raw.lineNo = pData->lineNo;
    info.raw.idxFunction = getStringIndex(pData->strIdxFunction);
    info.raw.idxFile = getStringIndex(pData->strIdxFile);
    info.raw.depth = pData->depth;

    auto& cmd = cmdInsertLockTry;
    cmd.bind(1, static_cast<int64_t>(pData->lockHandle));
    cmd.bind(2, static_cast<int32_t>(pData->threadID));
    cmd.bind(3, static_cast<int64_t>(pData->start));
    cmd.bind(4, static_cast<int64_t>(pData->end));
    cmd.bind(5, static_cast<int32_t>(pData->result));
    cmd.bind(6, static_cast<int64_t>(info.packed));
    cmd.bind(7, strIdx);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return getPacketSizeIncArgData(pData);
}

int32_t TraceBuilder::handleDataPacketLockState(const DataPacketLockState* pData)
{
    insertLockIfMissing(pData->lockHandle);

    StringBuf strBuf;
    int32_t strIdx = getStringIndex(strBuf, pData, sizeof(*pData), pData->strIdxFmt);

    PackedSourceInfo info;
    info.raw.lineNo = pData->lineNo;
    info.raw.idxFunction = getStringIndex(pData->strIdxFunction);
    info.raw.idxFile = getStringIndex(pData->strIdxFile);
    info.raw.depth = 0;

    auto& cmd = cmdInsertLockState;
    cmd.bind(1, static_cast<int64_t>(pData->lockHandle));
    cmd.bind(2, static_cast<int32_t>(pData->threadID));
    cmd.bind(3, static_cast<int64_t>(pData->time));
    cmd.bind(4, static_cast<int64_t>(pData->state));
    cmd.bind(5, static_cast<int64_t>(info.packed));
    cmd.bind(6, strIdx);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return getPacketSizeIncArgData(pData);
}

int32_t TraceBuilder::handleDataPacketLockSetName(const DataPacketLockSetName* pData)
{
    auto idxFmt = getStringIndex(pData->strIdxFmt);

    insertLockIfMissing(pData->lockHandle);

    auto& cmd = cmdInsertLockName;
    cmd.bind(1, static_cast<int64_t>(pData->lockHandle));
    cmd.bind(2, static_cast<int64_t>(pData->time)); 
    cmd.bind(3, idxFmt);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return getPacketSizeIncArgData(pData);
}

int32_t TraceBuilder::handleDataPacketThreadSetName(const DataPacketThreadSetName* pData)
{
    StringBuf strBuf;
    int32_t strIdx = getStringIndex(strBuf, pData, sizeof(*pData), pData->strIdxFmt);

    auto& cmd = cmdInsertThreadName;
    cmd.bind(1, static_cast<int32_t>(pData->threadID));
    cmd.bind(2, static_cast<int64_t>(pData->time));
    cmd.bind(3, strIdx);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return getPacketSizeIncArgData(pData);
}

int32_t TraceBuilder::handleDataPacketLockCount(const DataPacketLockCount* pData)
{
    X_UNUSED(pData);
    // not sure how best to store this just yet.

    return sizeof(*pData);
}

int32_t TraceBuilder::handleDataPacketMemAlloc(const DataPacketMemAlloc* pData)
{
    StringBuf strBuf;
    int32_t strIdx = getStringIndex(strBuf, pData, sizeof(*pData), pData->strIdxFmt);

    PackedSourceInfo info;
    info.raw.lineNo = pData->lineNo;
    info.raw.idxFunction = getStringIndex(pData->strIdxFunction);
    info.raw.idxFile = getStringIndex(pData->strIdxFile);
    info.raw.depth = 0;

    auto& cmd = cmdInsertMemory;
    cmd.bind(1, static_cast<int32_t>(pData->ptr));
    cmd.bind(2, static_cast<int32_t>(pData->size));
    cmd.bind(3, static_cast<int32_t>(pData->threadID));
    cmd.bind(4, static_cast<int64_t>(pData->time));
    cmd.bind(5, static_cast<int32_t>(MemOp::Alloc));
    cmd.bind(6, static_cast<int64_t>(info.packed));
    cmd.bind(7, strIdx);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return getPacketSizeIncArgData(pData);
}

int32_t TraceBuilder::handleDataPacketMemFree(const DataPacketMemFree* pData)
{
    PackedSourceInfo info;
    info.raw.lineNo = pData->lineNo;
    info.raw.idxFunction = getStringIndex(pData->strIdxFunction);
    info.raw.idxFile = getStringIndex(pData->strIdxFile);
    info.raw.depth = 0;

    auto& cmd = cmdInsertMemory;
    cmd.bind(1, static_cast<int32_t>(pData->ptr));
    cmd.bind(2, -1);
    cmd.bind(3, static_cast<int32_t>(pData->threadID));
    cmd.bind(4, static_cast<int64_t>(pData->time));
    cmd.bind(5, static_cast<int32_t>(MemOp::Free));
    cmd.bind(6, static_cast<int64_t>(info.packed));
    cmd.bind(7);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return sizeof(*pData);
}

int32_t TraceBuilder::handleDataPacketMessage(const DataPacketMessage* pData)
{
    StringBuf strBuf;
    int32_t strIdx = getStringIndex(strBuf, pData, sizeof(*pData), pData->strIdxFmt);

    auto& cmd = cmdInsertMessage;
    cmd.bind(1, static_cast<int64_t>(pData->time));
    cmd.bind(2, static_cast<int32_t>(pData->logType));
    cmd.bind(3, strIdx);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return getPacketSizeIncArgData(pData);
}

int32_t TraceBuilder::handleDataPacketPlot(const DataPacketPlot* pData)
{
    StringBuf strBuf;
    int32_t strIdx = getStringIndex(strBuf, pData, sizeof(*pData), pData->strIdxFmt);

    constexpr std::array<int32_t,6> typeSize = {
        sizeof(int32_t),
        sizeof(uint32_t),
        sizeof(int64_t),
        sizeof(uint64_t),
        sizeof(float),
        sizeof(double),
    };

    auto& cmd = cmdInsertPlot;
    cmd.bind(1, static_cast<int32_t>(pData->value.plotType));
    cmd.bind(2, static_cast<int64_t>(pData->time));
    cmd.bind(3, &pData->value.uint64, typeSize[pData->value.valueType]);
    cmd.bind(4, strIdx);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return getPacketSizeIncArgData(pData);
}

int32_t TraceBuilder::handleDataPacketCallStack(const DataPacketCallStack* pData)
{
    X_UNUSED(pData);

    const tt_int32 frameDataSize = sizeof(tt_uint64) * pData->numFrames;
    const tt_int32 dataSize = sizeof(*pData) + frameDataSize;
    const tt_uint64* pFrames = reinterpret_cast<const tt_uint64*>(pData + 1);

    auto& cmd = cmdInsertCallstack;
    cmd.bind(1, static_cast<int32_t>(pData->id));
    cmd.bind(2, static_cast<int32_t>(pData->numFrames));
    cmd.bind(3, pFrames, frameDataSize);
    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return dataSize;
}

int32_t TraceBuilder::handleDataPacketPDB(const DataPacketPDB* pData)
{
    StringBuf strBuf;
    int32_t strIdx = getStringIndex(strBuf, pData, sizeof(*pData), pData->strIdxName);

    auto& cmd = cmdInsertPDB;
    cmd.bind(1, static_cast<int64_t>(pData->modAddr));
    cmd.bind(2, static_cast<int32_t>(pData->imageSize));
    cmd.bind(3, pData->guid, sizeof(pData->guid));
    cmd.bind(4, strIdx);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return sizeof(*pData);
}


// --------------------------------

core::UniquePointer<ITelemServer> createServer(core::MemoryArenaBase* arena)
{
    return core::UniquePointer<ITelemServer>(core::makeUnique<Server>(arena, arena));
}

Server::Server(core::MemoryArenaBase* arena) :
    arena_(arena),
    apps_(arena)
{
    // TODO: better place?
    if (!winSockInit()) {
     
    }

#if 0
    DataPacketZone zone;
    zone.start = 0;
    zone.end = 1000;

    StringBuf str;

    ZoneTree tree;

    str.set("(goat/boat/moat)");
    tree.addZone(str, &zone);

    str.set("(goat/moat)");
    tree.addZone(str, &zone);

    str.set("(goat)");
    tree.addZone(str, &zone);

    str.set("(goat/pickle)");
    tree.addZone(str, &zone);

    str.set("(goat/meow?)");
    tree.addZone(str, &zone);

    str.set("(goat/defook)");
    tree.addZone(str, &zone);

    tree.addZone(str, &zone);
    tree.addZone(str, &zone);
    tree.addZone(str, &zone);
    tree.addZone(str, &zone);

    tree.print();

    ZoneTree::NodeFlatArr arr(g_TelemSrvLibArena);
    tree.getNodes(arr);
#endif

    // so need to save these to db

}

Server::~Server()
{
    winSockShutDown();
}

bool Server::loadApps()
{
    // iterator folders for apps.
    auto* pFileSys = gEnv->pFileSys;

    core::Path<> dirPath("traces");

    core::Path<> dirSearch(dirPath);
    dirSearch.ensureSlash();
    dirSearch.append("*");

    core::IFileSys::FindData fd;

    auto findPair = pFileSys->findFirst(dirSearch, fd);
    if (findPair.handle == core::IFileSys::INVALID_FIND_HANDLE) {
        if (findPair.valid) {
            X_WARNING("TelemSrv", "no apps found in: \"%s\"", dirPath.c_str());
            return true;
        }

        X_ERROR("TelemSrv", "Failed to iterate dir: \"%s\"", dirPath.c_str());
        return false;
    }

    do
    {
        if (fd.attrib.IsSet(core::FindData::AttrFlag::DIRECTORY)) {
            core::Path<> subDir(dirPath);
            subDir /= fd.name;

            // it's a app.
            loadAppTraces(fd.name, subDir);

            continue;
        }

    } while (pFileSys->findnext(findPair.handle, fd));

    pFileSys->findClose(findPair.handle);
    return true;
}

bool Server::loadAppTraces(core::Path<> appName, const core::Path<>& dir)
{
    TraceApp app(TelemFixedStr(appName.begin(), appName.end()), arena_);

    core::Path<> dirSearch(dir);
    dirSearch.ensureSlash();
    dirSearch.append("*.db");

    auto* pFileSys = gEnv->pFileSys;
    core::IFileSys::FindData fd;

    auto findPair = pFileSys->findFirst(dirSearch, fd);
    if (findPair.handle == core::IFileSys::INVALID_FIND_HANDLE) {
        if (findPair.valid) {
            X_WARNING("TelemSrv", "no traces found for app: \"%s\"", appName.c_str());
            return true;
        }

        X_ERROR("TelemSrv", "Failed to iterate dir: \"%s\"", dir.c_str());
        return false;
    }

    do
    {
        if (fd.attrib.IsSet(core::FindData::AttrFlag::DIRECTORY)) {
            continue;
        }

        Trace trace;
        trace.dbPath = dir / fd.name;

        // load info.
        // dunno how slow loading all the sql dbs will be probs not that slow.
        // will see..
        sql::SqlLiteDb db;

        if (!db.connect(trace.dbPath.c_str(), sql::OpenFlags())) {
            X_ERROR("TelemSrv", "Failed to openDB: \"%s\"", trace.dbPath.c_str());
            continue;
        }

        // load meta?
        bool loaded = true;

        core::string guidStr;
        core::string dateStr;

        loaded &= TraceDB::getMetaStr(db, "guid", guidStr);
        loaded &= TraceDB::getMetaStr(db, "dateStamp", dateStr);
        loaded &= TraceDB::getMetaStr(db, "hostName", trace.hostName);
        loaded &= TraceDB::getMetaStr(db, "buildInfo", trace.buildInfo);
        loaded &= TraceDB::getMetaStr(db, "cmdLine", trace.cmdLine);
        loaded &= TraceDB::getMetaUInt64(db, "tickPerMicro", trace.ticksPerMicro);
        loaded &= TraceDB::getMetaUInt64(db, "tickPerMs", trace.ticksPerMs);

        if (!loaded) {
            X_ERROR("TelemSrv", "Failed to load meta for: \"%s\"", trace.dbPath.c_str());
            continue;
        }

        if (!core::DateTimeStamp::fromString(core::string_view(dateStr), trace.date)) {
            continue;
        }

        trace.guid = core::Guid(core::string_view(guidStr));
        if (!trace.guid.isValid()) {
            continue;
        }

        app.traces.append(trace);

    } while (pFileSys->findnext(findPair.handle, fd));

    pFileSys->findClose(findPair.handle);

    X_LOG0("TelemSrv", "Added App \"%s\" %" PRIuS " Trace(s)", app.appName.c_str(), app.traces.size());

    X_LOG_BULLET;
    for (const auto& trace : app.traces)
    {
        X_LOG0("TelemSrv", "Trace \"%s\"", trace.dbPath.fileName());
    }

    apps_.push_back(std::move(app));
    return true;
}



void readfromIOCPJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pJobData)
{
    X_UNUSED(jobSys, threadIdx, pJob, pJobData);

    auto* pJobSys = gEnv->pJobSys;

    auto* pData = reinterpret_cast<const IOCPJobData*>(pJobData);

    ClientConnection* pClientCon = nullptr;
    PerClientIoData* pIOContext = nullptr;
    DWORD bytesTransferred = 0;
    DWORD flags = 0;

    lastErrorWSA::Description errDsc;

    while (1)
    {
        auto ok = GetQueuedCompletionStatus(
            pData->hIOCP,
            &bytesTransferred,
            (PDWORD_PTR)&pClientCon,
            (LPOVERLAPPED*)&pIOContext,
            INFINITE
        );

        if (!ok) {
            // rip
            continue;
        }

        if (bytesTransferred == 0) {
            // rip
            continue;
        }

        if (!pClientCon) {
            // rip
            return;
        }

        auto& ioCtx = *pIOContext;
        ioCtx.bytesTrans += bytesTransferred;

        X_LOG0("TelemSrv", "Recv %" PRIu32 " buffer has %" PRIu32, bytesTransferred, ioCtx.bytesTrans);


        // so this could either be me sending data to viewer or getting data from trace or viewer.
        // so first i need to know if we are reading or writing.
        // for revicing we basically need to wait till we have a full packet.
        if (ioCtx.op == IOOperation::Recv)
        {
            // i need to know if we have a full packet yet.
            if (ioCtx.bytesTrans >= sizeof(PacketBase))
            {
                auto* pHdr = reinterpret_cast<const PacketBase*>(ioCtx.buf.buf);
                if (pHdr->dataSize == 0) {
                    X_ERROR("TelemSrv", "Client sent packet with length zero...");
                    continue;
                }

                if (pHdr->dataSize > sizeof(ioCtx.recvbuf)) {
                    X_ERROR("TelemSrv", "Client sent oversied packet of size %i...", static_cast<int32_t>(pHdr->dataSize));
                    continue;
                }

                // we have the packet?
                if (pHdr->dataSize >= ioCtx.bytesTrans) {
                    const auto trailingBytes = ioCtx.bytesTrans - pHdr->dataSize;

                    // dispatch a job to process this bitch.
                    // we don't want to just keep allocating jobs for all the packets
                    // as we will jsut end up with far too many jobs.
                    // ideally we need to limit it, so i don't actually want to dispatch another WSARecv
                    // untill i have finished decompressing the last?
                    // but ideally we want the next one ready to go.
                    // but can't really do that, have to see what the bubble is like.
                    // then we insert the data, need to rate limit that also.


                    // so i need like a way to limit depth of pipelines.
                    // allmost like a queue.
                    // if we allow 3 uncompressed packets we dispatch network reads while that is still the case.
                    // that way the db insert should not starve and multiple packets can decompress
                    // actually no they can't since it's a stream must be done in order.
                    // so as long as decompressed packets is below 3 and one is not active decompress.
                    // dispatch a read?
                    
                    ProcessPacketJobData ppjd;
                    ppjd.pBuf = X_NEW_ARRAY(uint8_t, pHdr->dataSize, g_TelemSrvLibArena, "PacketBuf");
                    ppjd.length = pHdr->dataSize;

                    std::memcpy(ppjd.pBuf, ioCtx.recvbuf, pHdr->dataSize);

                    // dispatch the job.
                    pJobSys->CreateMemberJobAndRun<ClientConnection>(
                        pClientCon,
                        &ClientConnection::processNetPacketJob,
                        ppjd
                        JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL)
                    );

                    // shit trailing bytes to start.
                    if (trailingBytes)
                    {
                        const auto offset = pHdr->dataSize;
                        std::memcpy(ioCtx.recvbuf, &ioCtx.recvbuf[offset], trailingBytes);
                    }

                    // if trailingBytes is zero logic is still correct.
                    ioCtx.buf.buf = ioCtx.recvbuf + trailingBytes;
                    ioCtx.buf.len = sizeof(ioCtx.recvbuf) - trailingBytes;
                }

            }
            else
            {
                // need to read some more, shrink the buffer.
                ioCtx.buf.buf = ioCtx.recvbuf + ioCtx.bytesTrans;
                ioCtx.buf.len = sizeof(ioCtx.recvbuf) - ioCtx.bytesTrans;
            }
            
            auto res = platform::WSARecv(pClientCon->socket_, &ioCtx.buf, 1, &bytesTransferred, &flags, &ioCtx.overlapped, nullptr);
            if (res == SOCKET_ERROR) {
                auto err = lastErrorWSA::Get();
                if (err != ERROR_IO_PENDING) {
                    X_ERROR("TelemSrv", "failed to recv for client. Error: %s", lastErrorWSA::ToString(err, errDsc));
                }
            }

        }
        else if (pIOContext->op == IOOperation::Send)
        {

        }
    }
}

bool Server::listen(void)
{
    // completetion port.
    auto hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (hIOCP == nullptr) {
        core::lastError::Description Dsc;
        X_ERROR("TelemSrv", "failed to create iocp. Error: %s", core::lastError::ToString(Dsc));
        return false;
    }

    lastErrorWSA::Description errDsc;

    struct platform::addrinfo hints;
    struct platform::addrinfo *result = nullptr;

    core::zero_object(hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = platform::IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    int res = platform::getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (res != 0) {
        return false;
    }

    platform::SOCKET listenSocket = INV_SOCKET;
    platform::SOCKET clientSocket = INV_SOCKET;

    // Create a SOCKET for connecting to server
    listenSocket = platform::WSASocketW(result->ai_family, result->ai_socktype, result->ai_protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (listenSocket == INV_SOCKET) {
        platform::freeaddrinfo(result);
        return false;
    }

    int32_t sock_opt = 1024 * 1024;
    res = platform::setsockopt(listenSocket, SOL_SOCKET, SO_RCVBUF, (char*)&sock_opt, sizeof(sock_opt));
    if (res != 0) {
        X_ERROR("TelemSrv", "Failed to set rcvbuf on socket. Error: %s", lastErrorWSA::ToString(errDsc));
    }

    // Setup the TCP listening socket
    res = platform::bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (res == SOCKET_ERROR) {
        X_ERROR("TelemSrv", "bind failed. Error: %s", lastErrorWSA::ToString(errDsc));
        platform::freeaddrinfo(result);
        platform::closesocket(listenSocket);
        return false;
    }

    platform::freeaddrinfo(result);

    res = platform::listen(listenSocket, SOMAXCONN);
    if (res == SOCKET_ERROR) {
        X_ERROR("TelemSrv", "listen failed. Error: %s", lastErrorWSA::ToString(errDsc));
        platform::closesocket(listenSocket);
        return false;
    }

    // start a job to read from the queue.
    auto* pJobSys = gEnv->pJobSys;

    IOCPJobData data;
    data.hIOCP = hIOCP;

    auto* pJob = pJobSys->CreateJob(readfromIOCPJob, data JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));
    pJobSys->Run(pJob);


    while (true)
    {
        X_LOG0("TelemSrv", "Waiting for client on port: %s", DEFAULT_PORT);

        struct platform::sockaddr addr;
        int32_t addrLen = sizeof(addr);

        clientSocket = platform::WSAAccept(listenSocket, &addr, &addrLen, nullptr, 0);
        if (clientSocket == SOCKET_ERROR) {
            X_ERROR("TelemSrv", "accept failed. Error: %s", lastErrorWSA::ToString(errDsc));
            platform::freeaddrinfo(result);
            platform::closesocket(listenSocket);
            return false;
        }

        char hostname[NI_MAXHOST] = {};
        char servInfo[NI_MAXSERV] = {};

        res = platform::getnameinfo(
            &addr,
            addrLen,
            hostname,
            NI_MAXHOST,
            servInfo,
            NI_MAXSERV,
            NI_NUMERICSERV
        );

        if (res != 0) {
            X_ERROR("TelemSrv", "Error resolving client name. Error: %s", lastErrorWSA::ToString(errDsc));
        }

        X_LOG0("TelemSrv", "Client connected: %s:%s", hostname, servInfo);

        ClientConnection* pClientCon = new ClientConnection(*this, g_TelemSrvLibArena); // TEMP
        pClientCon->socket_ = clientSocket;
        memcpy(&pClientCon->clientAddr_, &addr, addrLen);
        pClientCon->host_.set(hostname);
        pClientCon->serv_.set(servInfo);


        hIOCP = CreateIoCompletionPort((HANDLE)clientSocket, hIOCP, (DWORD_PTR)pClientCon, 0);
        if (hIOCP == nullptr) {
            core::lastError::Description Dsc;
            X_ERROR("TelemSrv", "failed to create client iocp. Error: %s", core::lastError::ToString(Dsc));
            platform::freeaddrinfo(result);
            platform::closesocket(listenSocket);
            return false;
        }

        auto& io = pClientCon->io_;
        io.op = IOOperation::Recv;

        // wait for some data.
        DWORD flags = 0;
        DWORD recvBytes = 0;
        res = platform::WSARecv(clientSocket, &io.buf, 1, &recvBytes, &flags, &io.overlapped, nullptr);
        if (res == SOCKET_ERROR) {
            auto err = lastErrorWSA::Get();
            if (err != ERROR_IO_PENDING) {
                X_ERROR("TelemSrv", "failed to recv for client. Error: %s", lastErrorWSA::ToString(err, errDsc));
                return false;
            }
        }

    }

    return true;
}

void Server::addTraceForApp(const TelemFixedStr& appName, Trace& trace)
{
    core::CriticalSection::ScopedLock lock(cs_);

    auto it = std::find_if(apps_.begin(), apps_.end(), [&appName](const TraceApp& app) {
        return app.appName == appName;
    });

    TraceApp* pApp = nullptr;
    if (it == apps_.end())
    {
        apps_.emplace_back(appName, g_TelemSrvLibArena);
        pApp = &apps_.back();
    }
    else
    {
        pApp = it;
    }

    pApp->traces.append(trace);
}

// i need to send list of apps to client
bool Server::sendAppList(ClientConnection& client)
{
    int32_t numApps = static_cast<int32_t>(apps_.size());
    int32_t numTraces = core::accumulate(apps_.begin(), apps_.end(), 0_i32, [](const TraceApp& app) {
        return static_cast<int32_t>(app.traces.size());
    });

    AppsListHdr resHdr;
    resHdr.dataSize = static_cast<tt_uint16>(
        sizeof(resHdr) + 
        (sizeof(AppsListData) * numApps) +
        (sizeof(AppTraceListData) * numTraces)
    );
    resHdr.type = PacketType::AppList;
    resHdr.num = numApps;

    client.sendDataToClient(&resHdr, sizeof(resHdr));

    for (const auto& app : apps_)
    {
        AppsListData ald;
        ald.numTraces = static_cast<int32_t>(app.traces.size());
        strcpy_s(ald.appName, app.appName.c_str());

        client.sendDataToClient(&ald, sizeof(ald));

        // send the traces
        for (const auto& trace : app.traces)
        {
            AppTraceListData tld;
            tld.guid = trace.guid;
            tld.active = trace.active;
            tld.date = trace.date;
            strcpy_s(tld.hostName, trace.hostName.c_str());
            strcpy_s(tld.buildInfo, trace.buildInfo.c_str());

            client.sendDataToClient(&tld, sizeof(tld));
        }
    }

    return true;
}

void Server::handleQueryTraceInfo(ClientConnection& client, const QueryTraceInfo* pHdr)
{
    core::Guid::GuidStr guidStr;
    X_LOG0("TelemSrv", "Recived trace info request for: \"%s\"", pHdr->guid.toString(guidStr));

    for (auto& app : apps_)
    {
        for (auto& trace : app.traces)
        {
            if (trace.guid == pHdr->guid)
            {
                sql::SqlLiteDb db;

                if (!db.connect(trace.dbPath.c_str(), sql::OpenFlags())) {
                    X_ERROR("TelemSrv", "Failed to openDB: \"%s\"", trace.dbPath.c_str());
                    continue;
                }

                TraceStats stats;
                if (TraceDB::getStats(db, stats))
                {
                    QueryTraceInfoResp resp;
                    resp.type = PacketType::QueryTraceInfoResp;
                    resp.dataSize = sizeof(resp);
                    resp.guid = pHdr->guid;
                    resp.stats = stats;

                    client.sendDataToClient(&resp, sizeof(resp));
                }

                return;
            }
        }
    }
}

bool Server::getTraceForGuid(const core::Guid& guid, Trace& traceOut)
{
    core::CriticalSection::ScopedLock lock(cs_);

    for (auto& app : apps_)
    {
        for (auto& trace : app.traces)
        {
            if (trace.guid == guid)
            {
                traceOut = trace;
                return true;
            }
        }
    }
  
    return false;
}



X_NAMESPACE_END
