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

    void sendDataToClient(ClientConnection& client, const void* pData, size_t len)
    {
        // send some data...
        int res = platform::send(client.socket, reinterpret_cast<const char*>(pData), static_cast<int>(len), 0);
        if (res == SOCKET_ERROR) {
            X_LOG0("TelemSrv", "send failed with error: %d", platform::WSAGetLastError());
            return;
        }
    }

    void sendConnectionRejected(ClientConnection& client, const char* pReason)
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
        sendDataToClient(client, buf, datalen);
    }

    bool handleDataSream(ClientConnection& client, uint8_t* pData)
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

        auto* pDst = &client.cmpRingBuf[client.cmpBufBegin];

        int32_t cmpLenOut = static_cast<int32_t>(client.lz4DecodeStream.decompressContinue(pHdr + 1, pDst, origLen));
        if (cmpLenOut != cmpLen) {
            // TODO: ..
            return false;
        }

        client.cmpBufBegin += origLen;
        if (client.cmpBufBegin >= (COMPRESSION_RING_BUFFER_SIZE - COMPRESSION_MAX_INPUT_SIZE)) {
            client.cmpBufBegin = 0;
        }

        auto& strm = client.traceStrm;
        sql::SqlLiteTransaction trans(strm.db.con);

        // process this data?
        for (int32 i = 0; i < origLen; )
        {
            // packet me baby!
            auto* pPacket = reinterpret_cast<const DataPacketBase*>(&pDst[i]);

            switch (pPacket->type)
            {
                case DataStreamType::StringTableAdd:
                {
                    auto* pStr = reinterpret_cast<const DataPacketStringTableAdd*>(&pDst[i]);
                    strm.db.handleDataPacketStringTableAdd(pStr);
                    i += sizeof(DataPacketStringTableAdd);
                    i += pStr->length;
                    break;
                }
                case DataStreamType::Zone:
                {
                    strm.db.handleDataPacketZone(reinterpret_cast<const DataPacketZone*>(&pDst[i]));
                    i += sizeof(DataPacketZone);
                    break;
                }
                case DataStreamType::TickInfo:
                {
                    strm.db.handleDataPacketTickInfo(reinterpret_cast<const DataPacketTickInfo*>(&pDst[i]));
                    i += sizeof(DataPacketTickInfo);
                    break;
                }
                case DataStreamType::ThreadSetName:
                {
                    strm.db.handleDataPacketThreadSetName(reinterpret_cast<const DataPacketThreadSetName*>(&pDst[i]));
                    i += sizeof(DataPacketThreadSetName);
                    break;
                }
                case DataStreamType::CallStack:
                {
                    strm.db.handleDataPacketCallStack(reinterpret_cast<const DataPacketCallStack*>(&pDst[i]));
                    i += sizeof(DataPacketCallStack);
                    break;
                }
                case DataStreamType::LockSetName:
                {
                    strm.db.handleDataPacketLockSetName(reinterpret_cast<const DataPacketLockSetName*>(&pDst[i]));
                    i += sizeof(DataPacketLockSetName);
                    break;
                }
                case DataStreamType::LockTry:
                {
                    strm.db.handleDataPacketLockTry(reinterpret_cast<const DataPacketLockTry*>(&pDst[i]));
                    i += sizeof(DataPacketLockTry);
                    break;
                }
                case DataStreamType::LockState:
                {
                    strm.db.handleDataPacketLockState(reinterpret_cast<const DataPacketLockState*>(&pDst[i]));
                    i += sizeof(DataPacketLockState);
                    break;
                }
                case DataStreamType::LockCount:
                {
                    strm.db.handleDataPacketLockCount(reinterpret_cast<const DataPacketLockCount*>(&pDst[i]));
                    i += sizeof(DataPacketLockCount);
                    break;
                }
                case DataStreamType::MemAlloc:
                {
                    strm.db.handleDataPacketMemAlloc(reinterpret_cast<const DataPacketMemAlloc*>(&pDst[i]));
                    i += sizeof(DataPacketMemAlloc);
                    break;
                }
                case DataStreamType::MemFree:
                {
                    strm.db.handleDataPacketMemFree(reinterpret_cast<const DataPacketMemFree*>(&pDst[i]));
                    i += sizeof(DataPacketMemFree);
                    break;
                }

                default:
                    X_NO_SWITCH_DEFAULT_ASSERT;
            }
        }

        trans.commit();

        return true;
    }

    bool getStats(sql::SqlLiteDb& db, TraceStats& stats)
    {
        // These need to be fast even when there is 10 million rows etc..
        {
            sql::SqlLiteQuery qry(db, "SELECT MAX(_rowid_) FROM zones LIMIT 1");
            auto it = qry.begin();
            if (it == qry.end()) {
                X_ERROR("TelemSrv", "Failed to load zone counts");
                return false;
            }

            stats.numZones = (*it).get<int64_t>(0);
        }

        {
            sql::SqlLiteQuery qry(db, "SELECT MAX(_rowid_) FROM ticks LIMIT 1");
            auto it = qry.begin();
            if (it == qry.end()) {
                X_ERROR("TelemSrv", "Failed to load tick counts");
                return false;
            }

            stats.numTicks = (*it).get<int64_t>(0);
        }

        {
            sql::SqlLiteQuery qry(db, "SELECT endMicro FROM ticks WHERE _rowid_ = (SELECT MAX(_rowid_) FROM ticks)");
            auto it = qry.begin();
            if (it == qry.end()) {
                X_ERROR("TelemSrv", "Failed to load tick counts");
                return false;
            }

            stats.durationMicro = (*it).get<int64_t>(0);
        }

        {
            // simular performance.
            // SELECT * FROM zones WHERE _rowid_ = (SELECT MAX(_rowid_) FROM zones);
            // SELECT * FROM zones ORDER BY Id DESC LIMIT 1;
            //sql::SqlLiteQuery qry(db, "SELECT * FROM zones WHERE _rowid_ = (SELECT MAX(_rowid_) FROM zones)");

        }

        return true;
    }

    bool getMetaStr(sql::SqlLiteDb& db, const char* pName, core::string& strOut)
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

    bool getMetaUInt64(sql::SqlLiteDb& db, const char* pName, uint64_t& valOut)
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

} // namespace



bool TraceDB::createDB(core::Path<char>& path)
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

    cmdInsertZone.prepare("INSERT INTO zones (threadID, startTick, endTick, sourceInfoIdx, stackDepth) VALUES(?,?,?,?,?)");
    cmdInsertString.prepare("INSERT INTO strings (Id, value) VALUES(?, ?)");
    cmdInsertTickInfo.prepare("INSERT INTO ticks (threadId, startTick, endTick, startMicro, endMicro) VALUES(?,?,?,?,?)");
    cmdInsertLock.prepare("INSERT INTO locks (Id) VALUES(?)");
    cmdInsertLockTry.prepare("INSERT INTO lockTry (lockId, threadId, start, end, descriptionStrId) VALUES(?,?,?,?,?)");
    cmdInsertLockState.prepare("INSERT INTO lockStates (lockId, threadId, time, state) VALUES(?,?,?,?)");
    cmdInsertLockName.prepare("INSERT INTO lockNames (lockId, time, nameStrId) VALUES(?,?,?)");
    cmdInsertThreadName.prepare("INSERT INTO threadNames (threadId, time, nameStrId) VALUES(?,?,?)");
    cmdInsertMeta.prepare("INSERT INTO meta (name, value) VALUES(?,?)");
    cmdInsertMemory.prepare("INSERT INTO memory (allocId, size, threadId, time, operation) VALUES(?,?,?,?,?)");
    return true;
}

bool TraceDB::openDB(core::Path<char>& path)
{
    if (!con.connect(path.c_str(), sql::OpenFlags())) {
        return false;
    }

    if (!createIndexes()) {
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

bool TraceDB::createIndexes(void)
{
    sql::SqlLiteCmd cmd(con, R"(
        CREATE INDEX IF NOT EXISTS "zones_start" ON "zones" (
            "start"	ASC
        )
    )");

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "Failed to creat indexes");
        return false;
    }

    return true;
}


bool TraceDB::createTables(void)
{
    if (!con.execute(R"(

CREATE TABLE IF NOT EXISTS "meta" (
	"Id"	        INTEGER,
	"name"	        TEXT NOT NULL UNIQUE,
	"value"	        TEXT NOT NULL,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "ticks" (
	"Id"	        INTEGER,
	"threadId"	    INTEGER NOT NULL,
	"startTick"	    INTEGER NOT NULL,
	"endTick"       INTEGER NOT NULL,
	"startMicro"	INTEGER NOT NULL,
	"endMicro"	    INTEGER NOT NULL,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "threadNames" (
    "Id"            INTEGER,
    "threadId"      INTEGER NOT NULL,
    "time"          INTEGER NOT NULL,
    "nameStrId"     INTEGER NOT NULL,
    PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "strings" (
	"Id"	        INTEGER,
	"value"	        TEXT NOT NULL UNIQUE,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "sourceInfo" (
	"Id"	        INTEGER,
	"fileStrId"	    INTEGER NOT NULL,
	"functionStrId"	INTEGER NOT NULL,
	"lineNo"	    INTEGER NOT NULL,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "zones" (
    "id"	        INTEGER,
    "threadId"	    INTEGER NOT NULL,
    "startTick"	    INTEGER NOT NULL,
    "endTick"	    INTEGER NOT NULL,
    "sourceInfoIdx"	INTEGER NOT NULL,
    "stackDepth"	INTEGER NOT NULL,
    PRIMARY KEY("id")
);

CREATE TABLE IF NOT EXISTS "locks" (
	"Id"	        INTEGER,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "lockNames" (
    "Id"            INTEGER,
    "lockId"          INTEGER NOT NULL,
    "time"          INTEGER NOT NULL,
    "nameStrId"     INTEGER NOT NULL,
    PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "lockTry" (
	"Id"	            INTEGER,
	"lockId"	        INTEGER NOT NULL,
    "threadId"	        INTEGER NOT NULL,
	"start"	            INTEGER NOT NULL,
	"end"	            INTEGER NOT NULL,
	"descriptionStrId"	INTEGER,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "lockStates" (
	"Id"	        INTEGER,
	"lockId"	    INTEGER NOT NULL,
	"threadId"	    INTEGER NOT NULL,
	"time"	        INTEGER NOT NULL,
	"state"	        INTEGER NOT NULL,
	PRIMARY KEY("Id")
);

CREATE TABLE "memory" (
	"Id"	        INTEGER,
	"allocId"	    INTEGER NOT NULL,
	"size"	        INTEGER NOT NULL,
	"threadId"	    INTEGER NOT NULL,
	"time"	        INTEGER NOT NULL,
	"operation"	    INTEGER NOT NULL,
	PRIMARY KEY("Id")
);


            )")) {
        X_ERROR("TelemSrv", "Failed to create tables");
        return false;
    }

    return true;
}

template<typename T>
bool TraceDB::setMeta(const char* pName, T value)
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

void TraceDB::insertLockIfMissing(uint64_t lockHandle)
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

void TraceDB::handleDataPacketTickInfo(const DataPacketTickInfo* pData)
{
    auto& cmd = cmdInsertTickInfo;
    cmd.bind(1, static_cast<int32_t>(pData->threadID));
    cmd.bind(2, static_cast<int64_t>(pData->start));
    cmd.bind(3, static_cast<int64_t>(pData->end));
    cmd.bind(4, static_cast<int64_t>(pData->startMicro));
    cmd.bind(5, static_cast<int64_t>(pData->endMicro));

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
}

void TraceDB::handleDataPacketStringTableAdd(const DataPacketStringTableAdd* pData)
{
    const char* pString = reinterpret_cast<const char*>(pData + 1);

    auto& cmd = cmdInsertString;
    cmd.bind(1, static_cast<int32_t>(pData->id));
    cmd.bind(2, pString, static_cast<size_t>(pData->length));

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
}

void TraceDB::handleDataPacketZone(const DataPacketZone* pData)
{
    PacketSourceInfo info;
    info.raw.lineNo = pData->lineNo;
    info.raw.idxFunction = pData->strIdxFunction.index;
    info.raw.idxFile = pData->strIdxFile.index;

    uint64_t sourceInfo = info.packed;

    auto& cmd = cmdInsertZone;
    cmd.bind(1, static_cast<int32_t>(pData->threadID));
    cmd.bind(2, static_cast<int64_t>(pData->start));
    cmd.bind(3, static_cast<int64_t>(pData->end));
    cmd.bind(4, static_cast<int64_t>(sourceInfo));
    cmd.bind(5, pData->stackDepth);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
}

void TraceDB::handleDataPacketLockTry(const DataPacketLockTry* pData)
{
    insertLockIfMissing(pData->lockHandle);

    auto& cmd = cmdInsertLockTry;
    cmd.bind(1, static_cast<int64_t>(pData->lockHandle));
    cmd.bind(2, static_cast<int32_t>(pData->threadID));
    cmd.bind(3, static_cast<int64_t>(pData->start));
    cmd.bind(4, static_cast<int64_t>(pData->end));
    cmd.bind(5, static_cast<int32_t>(pData->strIdxDescrption.index));

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
}

void TraceDB::handleDataPacketLockState(const DataPacketLockState* pData)
{
    insertLockIfMissing(pData->lockHandle);

    auto& cmd = cmdInsertLockState;
    cmd.bind(1, static_cast<int64_t>(pData->lockHandle));
    cmd.bind(2, static_cast<int32_t>(pData->threadID));
    cmd.bind(3, static_cast<int64_t>(pData->time));
    cmd.bind(4, static_cast<int64_t>(pData->state));

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
}

void TraceDB::handleDataPacketLockSetName(const DataPacketLockSetName* pData)
{
    insertLockIfMissing(pData->lockHandle);

    auto& cmd = cmdInsertLockName;
    cmd.bind(1, static_cast<int64_t>(pData->lockHandle));
    cmd.bind(2, static_cast<int64_t>(pData->time));
    cmd.bind(3, static_cast<int32_t>(pData->strIdxName.index));

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
}

void TraceDB::handleDataPacketThreadSetName(const DataPacketThreadSetName* pData)
{
    auto& cmd = cmdInsertThreadName;
    cmd.bind(1, static_cast<int32_t>(pData->threadID));
    cmd.bind(2, static_cast<int64_t>(pData->time));
    cmd.bind(3, static_cast<int32_t>(pData->strIdxName.index));

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
}

void TraceDB::handleDataPacketLockCount(const DataPacketLockCount* pData)
{
    X_UNUSED(pData);
    // not sure how best to store this just yet.
}

void TraceDB::handleDataPacketMemAlloc(const DataPacketMemAlloc* pData)
{
    auto& cmd = cmdInsertMemory;
    cmd.bind(1, static_cast<int32_t>(pData->ptr));
    cmd.bind(2, static_cast<int32_t>(pData->size));
    cmd.bind(3, static_cast<int32_t>(pData->threadID));
    cmd.bind(4, static_cast<int64_t>(pData->time));
    cmd.bind(5, static_cast<int32_t>(MemOp::Alloc));

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
}

void TraceDB::handleDataPacketMemFree(const DataPacketMemFree* pData)
{
    auto& cmd = cmdInsertMemory;
    cmd.bind(1, static_cast<int32_t>(pData->ptr));
    cmd.bind(2, -1);
    cmd.bind(3, static_cast<int32_t>(pData->threadID));
    cmd.bind(4, static_cast<int64_t>(pData->time));
    cmd.bind(5, static_cast<int32_t>(MemOp::Free));

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
}

void TraceDB::handleDataPacketCallStack(const DataPacketCallStack* pData)
{
    X_UNUSED(pData);

    // TODO: ...
}


bool TraceDB::getTicks(core::Array<DataPacketTickInfo>& ticks, int32_t startIdx, int32_t num)
{
    sql::SqlLiteQuery qry(con, "SELECT * FROM ticks LIMIT ? OFFSET ?");
    qry.bind(1, num);
    qry.bind(2, startIdx);

    auto it = qry.begin();
    if (it != qry.end()) {
        auto row = *it;

        DataPacketTickInfo tick;
        auto id = row.get<int32_t>(0);
        X_UNUSED(id);
        tick.threadID = static_cast<uint32_t>(row.get<int32_t>(1));
        tick.start = static_cast<uint64_t>(row.get<int64_t>(2));
        tick.end = static_cast<uint64_t>(row.get<int64_t>(3));
        tick.startMicro = static_cast<uint64_t>(row.get<int64_t>(4));
        tick.endMicro = static_cast<uint64_t>(row.get<int64_t>(5));

        ticks.append(tick);
    }

    return true;
}

bool TraceDB::getZones(core::Array<DataPacketZone>& zones, uint64_t tickBegin, uint64_t tickEnd)
{
    sql::SqlLiteQuery qry(con, "SELECT threadId, start, end, sourceInfoIdx, stackDepth FROM zones WHERE start >= ? AND start < ?");
    qry.bind(1, static_cast<int64_t>(tickBegin));
    qry.bind(2, static_cast<int64_t>(tickEnd));

    // how to send this back?
    // hot like a potato
    // do want to make new types for query api?
    // not really but then we have this goaty prefix on all the types
    // maybe it would be better to split it out :(

    auto it = qry.begin();
    if (it != qry.end()) {
        auto row = *it;

        PacketSourceInfo srcInfo;

        DataPacketZone zone;
        zone.threadID = static_cast<uint32_t>(row.get<int32_t>(0));
        zone.start = static_cast<uint64_t>(row.get<int64_t>(1));
        zone.end = static_cast<uint64_t>(row.get<int64_t>(2));
        srcInfo.packed = row.get<int32_t>(3);
        zone.stackDepth = static_cast<uint8_t>(row.get<int32_t>(4));

        zone.lineNo = srcInfo.raw.lineNo;
        zone.strIdxFile.index = srcInfo.raw.idxFile;
        zone.strIdxZone.index = srcInfo.raw.idxFunction;

        zones.append(zone);
    }

    return true;
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

        loaded &= getMetaStr(db, "guid", guidStr);
        loaded &= getMetaStr(db, "dateStamp", dateStr);
        loaded &= getMetaStr(db, "hostName", trace.hostName);
        loaded &= getMetaStr(db, "buildInfo", trace.buildInfo);
        loaded &= getMetaStr(db, "cmdLine", trace.cmdLine);
        loaded &= getMetaUInt64(db, "tickPerMicro", trace.ticksPerMicro);

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

    apps_.push_back(std::move(app));
    return true;
}

bool Server::listen(void)
{
    // completetion port.
    // auto hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

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
    listenSocket = platform::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INV_SOCKET) {
        platform::freeaddrinfo(result);
        return false;
    }

    int32_t sock_opt = 1024 * 512;
    res = platform::setsockopt(listenSocket, SOL_SOCKET, SO_RCVBUF, (char*)&sock_opt, sizeof(sock_opt));
    if (res != 0) {
        X_ERROR("TelemSrv", "Failed to set rcvbuf on socket. Error: %d", platform::WSAGetLastError());
    }

    // Setup the TCP listening socket
    res = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (res == SOCKET_ERROR) {
        X_ERROR("TelemSrv", "bind failed with error: %d", platform::WSAGetLastError());
        platform::freeaddrinfo(result);
        platform::closesocket(listenSocket);
        return false;
    }

    platform::freeaddrinfo(result);

    res = platform::listen(listenSocket, SOMAXCONN);
    if (res == SOCKET_ERROR) {
        platform::closesocket(listenSocket);
        return false;
    }

    while (true)
    {
        X_LOG0("TelemSrv", "Waiting for client on port: %s", DEFAULT_PORT);

        struct platform::sockaddr addr;
        int32_t addrLen = sizeof(addr);

        // Accept a client socket
        clientSocket = platform::accept(listenSocket, &addr, &addrLen);
        if (clientSocket == INV_SOCKET) {
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
            X_ERROR("TelemSrv", "Error resolving client name: %d", platform::WSAGetLastError());
        }

        X_LOG0("TelemSrv", "Client connected: %s:%s", hostname, servInfo);

        ClientConnection client(arena_);
        client.socket = clientSocket;
        client.hostName = hostname;
 
        handleClient(client);

        platform::closesocket(clientSocket);
    }

    // clean up socket.
    platform::closesocket(listenSocket);

    return true;
}

bool Server::processPacket(ClientConnection& client, uint8_t* pData)
{
    auto* pPacketHdr = reinterpret_cast<const PacketBase*>(pData);

    switch (pPacketHdr->type)
    {
        case PacketType::ConnectionRequest:
            return handleConnectionRequest(client, pData);
        case PacketType::ConnectionRequestViewer:
            return handleConnectionRequestViewer(client, pData);
        case PacketType::DataStream:
            return handleDataSream(client, pData);

        // From viewer clients.
        case PacketType::QueryTraceInfo:
            return handleQueryTraceInfo(client, pData);
        case PacketType::OpenTrace:
            return handleOpenTrace(client, pData);
        case PacketType::ReqTraceTicks:
            return handleReqTraceTicks(client, pData);
        case PacketType::ReqTraceZones:
            return handleReqTraceZones(client, pData);
        case PacketType::ReqTraceStrings:
            return handleReqTraceStrings(client, pData);
        default:
            X_ERROR("TelemSrv", "Unknown packet type %" PRIi32, static_cast<int>(pPacketHdr->type));
            return false;
    }
}

void Server::handleClient(ClientConnection& client)
{
    char recvbuf[MAX_PACKET_SIZE];

    while (1)
    {
        int recvbuflen = sizeof(recvbuf);
        if (!readPacket(client, recvbuf, recvbuflen)) {
            X_LOG0("TelemSrv", "Error reading packet");
            return;
        }

        //   X_LOG0("TelemSrv", "Bytes received: %" PRIi32, recvbuflen);

        if (!processPacket(client, reinterpret_cast<uint8_t*>(recvbuf))) {
            X_LOG0("TelemSrv", "Failed to process packet");
            return;
        }
    }
}

bool Server::handleConnectionRequest(ClientConnection& client, uint8_t* pData)
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
        sendConnectionRejected(client, "Client server version incompatible");
        return false;
    }
    if (pConReq->appNameLen < 1) {
        sendConnectionRejected(client, "Invalid app name");
        return false;
    }

    client.clientVer = pConReq->clientVer;

    auto* pStrData = reinterpret_cast<const char*>(pConReq + 1);
    auto* pAppNameStr = pStrData;
    auto* pBuildInfoStr = pAppNameStr + pConReq->appNameLen;
    auto* pCmdLineStr = pBuildInfoStr + pConReq->buildInfoLen;

    // Get the app name and see if we have it already.
    TelemFixedStr appName;
    appName.set(pAppNameStr, pAppNameStr + pConReq->appNameLen);

    auto it = std::find_if(apps_.begin(), apps_.end(), [&appName](const TraceApp& app) {
        return app.appName == appName;
    });

    TraceApp* pApp = nullptr;
    if (it == apps_.end())
    {
        apps_.emplace_back(appName, arena_);
        pApp = &apps_.back();
    }
    else
    {
        pApp = it;
    }

    X_ASSERT_NOT_NULL(pApp);
    X_ASSERT(pApp->appName.isNotEmpty(), "")();

    // Create a new trace 
    Trace trace;
    trace.guid = core::Guid::newGuid();
    trace.buildInfo.assign(pBuildInfoStr, pConReq->buildInfoLen);
    trace.cmdLine.assign(pCmdLineStr, pConReq->cmdLineLen);
    trace.ticksPerMicro = pConReq->ticksPerMicro;

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
    dbPath.append(pApp->appName.begin(), pApp->appName.end());
    dbPath.ensureSlash();
    // want like hostname or everything in on folder?
    dbPath.append("telem_");
    dbPath.append(date.toString(dateStr));
    dbPath.setExtension("db");

    if (!gEnv->pFileSys->directoryExists(dbPath, core::VirtualDirectory::BASE)) {
        if (!gEnv->pFileSys->createDirectoryTree(dbPath, core::VirtualDirectory::BASE)) {
            return false;
        }
    }

    trace.dbPath = dbPath;
    trace.hostName = client.hostName;

    // open a trace stream for the conneciton.
    auto& strm = client.traceStrm;
    if (!strm.db.createDB(dbPath)) {
        return false;
    }

    bool setMeta = true;

    VersionInfo::Description verStr;
    core::Guid::GuidStr guidStr;
    setMeta &= strm.db.setMeta("guid", trace.guid.toString(guidStr));
    setMeta &= strm.db.setMeta("appName", core::string_view(pApp->appName));
    setMeta &= strm.db.setMeta("hostName", core::string_view(client.hostName));
    setMeta &= strm.db.setMeta("buildInfo", trace.buildInfo);
    setMeta &= strm.db.setMeta("cmdLine", trace.cmdLine);
    setMeta &= strm.db.setMeta("dateStamp", dateStr);
    setMeta &= strm.db.setMeta("clientVer", client.clientVer.toString(verStr));
    setMeta &= strm.db.setMeta("serverVer", serverVer.toString(verStr));
    setMeta &= strm.db.setMeta<int64_t>("tickPerMicro", static_cast<int64_t>(trace.ticksPerMicro));

    if (!setMeta) {
        return false;
    }

    pApp->traces.push_back(trace);

    // Meow...
    X_LOG0("TelemSrv", "ConnectionAccepted:");
    X_LOG0("TelemSrv", "> AppName: %s", pApp->appName.c_str());
    X_LOG0("TelemSrv", "> BuildInfo: %s", trace.buildInfo.c_str());
    X_LOG0("TelemSrv", "> CmdLine: %s", trace.cmdLine.c_str());
    X_LOG0("TelemSrv", "> DB: %s", dbPath.c_str());

    // send a packet back!
    ConnectionRequestAcceptedHdr cra;
    core::zero_object(cra);
    cra.dataSize = sizeof(cra);
    cra.type = PacketType::ConnectionRequestAccepted;
    cra.serverVer = serverVer;

    sendDataToClient(client, &cra, sizeof(cra));
    return true;
}

bool Server::handleConnectionRequestViewer(ClientConnection& client, uint8_t* pData)
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
        sendConnectionRejected(client, "Viewer version incompatible with server");
        return false;
    }

    client.clientVer = pConReq->viewerVer;

    // Meow...
    X_LOG0("TelemSrv", "ConnectionAccepted(Viewer):");

    // send a packet back!
    ConnectionRequestAcceptedHdr cra;
    core::zero_object(cra);
    cra.dataSize = sizeof(cra);
    cra.type = PacketType::ConnectionRequestAccepted;
    cra.serverVer = serverVer;

    sendDataToClient(client, &cra, sizeof(cra));

    // send them some data.
    if (!sendAppList(client)) {
        X_LOG0("TelemSrv", "Error sending app list to client");
    }

    return true;
}

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

    sendDataToClient(client, &resHdr, sizeof(resHdr));

    for (const auto& app : apps_)
    {
        AppsListData ald;
        ald.numTraces = static_cast<int32_t>(app.traces.size());
        strcpy_s(ald.appName, app.appName.c_str());

        sendDataToClient(client, &ald, sizeof(ald));

        // send the traces
        for (const auto& trace : app.traces)
        {
            AppTraceListData tld;
            tld.guid = trace.guid;
            tld.active = trace.active;
            tld.date = trace.date;
            strcpy_s(tld.hostName, trace.hostName.c_str());
            strcpy_s(tld.buildInfo, trace.buildInfo.c_str());

            sendDataToClient(client, &tld, sizeof(tld));
        }
    }

    return true;
}

bool Server::handleQueryTraceInfo(ClientConnection& client, uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const QueryTraceInfo*>(pData);
    if (pHdr->type != PacketType::QueryTraceInfo) {
        X_ASSERT_UNREACHABLE();
    }

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
                if (getStats(db, stats))
                {
                    QueryTraceInfoResp resp;
                    resp.type = PacketType::QueryTraceInfoResp;
                    resp.dataSize = sizeof(resp);
                    resp.guid = pHdr->guid;
                    resp.stats.numZones = stats.numZones;
                    resp.stats.numTicks = stats.numTicks;
                    // resp.stats.numAllocations = stats.numAllocations;
                    resp.stats.durationMicro = stats.durationMicro;

                    sendDataToClient(client, &resp, sizeof(resp));
                }

                return true;
            }
        }
    }

    // you silly goat.
    return true;
}

bool Server::handleOpenTrace(ClientConnection& client, uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const OpenTrace*>(pData);
    if (pHdr->type != PacketType::OpenTrace) {
        X_ASSERT_UNREACHABLE();
    }

    OpenTraceResp otr;
    otr.dataSize = sizeof(otr);
    otr.type = PacketType::OpenTraceResp;
    otr.guid = pHdr->guid;

    // TODO: check we don't have it open already.
    // TODO: thread safety etc..
    for (size_t i = 0; i < client.traces.size();i++)
    {
        auto& trace = client.traces[i];
        if (trace.pTrace->guid == pHdr->guid)
        {
            X_WARNING("TelemSrv", "Client opened a trace they already have open");
            otr.handle = safe_static_cast<int8_t>(i);
            sendDataToClient(client, &otr, sizeof(otr));
            return true;
        }
    }

    if (client.traces.size() < MAX_TRACES_OPEN_PER_CLIENT)
    {
        for (auto& app : apps_)
        {
            for (auto& trace : app.traces)
            {
                if (trace.guid == pHdr->guid) 
                {
                    // we found it !
                    TraceStream ts;
                    ts.pTrace = &trace;
                    if (ts.db.openDB(trace.dbPath)) {
                        auto id = client.traces.size();
                        client.traces.emplace_back(std::move(ts));

                        otr.handle = safe_static_cast<int8_t>(id);
                        sendDataToClient(client, &otr, sizeof(otr));
                        return true;
                    }
                }
            }
        }
    }

    otr.handle = -1_ui8;
    sendDataToClient(client, &otr, sizeof(otr));
    return true;
}


void flushCompressionBuffer(ClientConnection& client)
{
    // compress it.
    const auto* pInBegin = &client.cmpRingBuf[client.cmpBufBegin];
    const size_t inBytes = client.cmpBufEnd - client.cmpBufBegin;

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

    const size_t cmpBytes = client.lz4Stream.compressContinue(
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

    sendDataToClient(client, cmpBuf, totalLen);

    client.cmpBufBegin = client.cmpBufEnd;
    if ((sizeof(client.cmpRingBuf) - client.cmpBufBegin) < COMPRESSION_MAX_INPUT_SIZE) {
        client.cmpBufBegin = 0;
        client.cmpBufEnd = 0;
    }
}


void addToCompressionBuffer(ClientConnection& client, const void* pData, int32_t len)
{
#if X_DEBUG
    if (len > COMPRESSION_MAX_INPUT_SIZE) {
        ::DebugBreak();
    }
#endif // X_DEBUG

    // can we fit this data?
    const int32_t space = COMPRESSION_MAX_INPUT_SIZE - (client.cmpBufEnd - client.cmpBufBegin);
    if (space < len) {
        flushCompressionBuffer(client);
    }

    memcpy(&client.cmpRingBuf[client.cmpBufEnd], pData, len);
    client.cmpBufEnd += len;
}

bool Server::handleReqTraceTicks(ClientConnection& client, uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceTicks*>(pData);
    if (pHdr->type != PacketType::ReqTraceTicks) {
        X_ASSERT_UNREACHABLE();
    }

    // MEOW
    // load me the ticks!
    int32_t handle = pHdr->handle;
    if (handle <= 0 || handle >= static_cast<int32_t>(client.traces.size())) {
        return false;
    }


    auto& ts = client.traces[pHdr->handle];

    // we just stream the rows to the client.
    // if there is loads of data we just send multiple compressed packets.
    const int32_t numToReturn = pHdr->num;

    sql::SqlLiteQuery qry(ts.db.con, "SELECT threadId, startTick, endTick, startMicro, endMicro FROM ticks LIMIT ? OFFSET ?");
    qry.bind(1, pHdr->tickIdx);
    qry.bind(2, numToReturn);

    auto it = qry.begin();
    if (it != qry.end()) {
        auto row = *it;

        DataPacketTickInfo tick;
        tick.type = DataStreamType::TickInfo;
        tick.threadID = static_cast<uint32_t>(row.get<int32_t>(0));
        tick.start = static_cast<uint64_t>(row.get<int64_t>(1));
        tick.end = static_cast<uint64_t>(row.get<int64_t>(2));
        tick.startMicro = static_cast<uint64_t>(row.get<int64_t>(3));
        tick.endMicro = static_cast<uint64_t>(row.get<int64_t>(4));

        addToCompressionBuffer(client, &tick, sizeof(tick));
    }

    flushCompressionBuffer(client);
    return true;
}

bool Server::handleReqTraceZones(ClientConnection& client, uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceZones*>(pData);
    if (pHdr->type != PacketType::ReqTraceZones) {
        X_ASSERT_UNREACHABLE();
    }

    int32_t handle = pHdr->handle;
    if (handle <= 0 || handle >= static_cast<int32_t>(client.traces.size())) {
        return false;
    }

    auto& ts = client.traces[pHdr->handle];

    sql::SqlLiteQuery qry(ts.db.con, "SELECT threadId, startTick, endTick, stackDepth FROM zones WHERE start >= ? AND end < ?");
    qry.bind(1, pHdr->start);
    qry.bind(2, pHdr->end);

    auto it = qry.begin();
    if (it != qry.end()) {
        auto row = *it;

        DataPacketZone zone;
        zone.type = DataStreamType::Zone;
        zone.threadID = static_cast<uint32_t>(row.get<int32_t>(0));
        zone.start = static_cast<uint64_t>(row.get<int64_t>(1));
        zone.end = static_cast<uint64_t>(row.get<int64_t>(2));
        zone.stackDepth = static_cast<tt_int8>(row.get<int32_t>(3));
        // TODO: finish

        addToCompressionBuffer(client, &zone, sizeof(zone));
    }

    flushCompressionBuffer(client);
    return true;
}

bool Server::handleReqTraceStrings(ClientConnection& client, uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceStrings*>(pData);
    if (pHdr->type != PacketType::ReqTraceZones) {
        X_ASSERT_UNREACHABLE();
    }

    int32_t handle = pHdr->handle;
    if (handle <= 0 || handle >= static_cast<int32_t>(client.traces.size())) {
        return false;
    }

    auto& ts = client.traces[pHdr->handle];

    sql::SqlLiteQuery qry(ts.db.con, "SELECT id, value FROM strings");

    auto it = qry.begin();
    if (it != qry.end()) {
        auto row = *it;

        int32_t id = row.get<int32_t>(0);
        int32_t strLen = row.columnBytes(1);
        const char* pStr = row.get<const char*>(1);

        DataPacketStringTableAdd strAdd;
        strAdd.type = DataStreamType::StringTableAdd;
        strAdd.id = static_cast<uint16_t>(id);
        strAdd.length = static_cast<uint16_t>(strLen);

        addToCompressionBuffer(client, &strAdd, sizeof(strAdd));
        addToCompressionBuffer(client, pStr, strLen);
    }

    flushCompressionBuffer(client);
    return true;
}




X_NAMESPACE_END