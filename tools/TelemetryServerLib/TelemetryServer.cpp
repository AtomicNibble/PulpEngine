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
                    i += strm.db.handleDataPacketZone(reinterpret_cast<const DataPacketZone*>(&pDst[i]));
                    break;
                }
                case DataStreamType::TickInfo:
                {
                    i += strm.db.handleDataPacketTickInfo(reinterpret_cast<const DataPacketTickInfo*>(&pDst[i]));
                    break;
                }
                case DataStreamType::ThreadSetName:
                {
                    i += strm.db.handleDataPacketThreadSetName(reinterpret_cast<const DataPacketThreadSetName*>(&pDst[i]));
                    break;
                }
                case DataStreamType::CallStack:
                {
                    i += strm.db.handleDataPacketCallStack(reinterpret_cast<const DataPacketCallStack*>(&pDst[i]));
                    break;
                }
                case DataStreamType::LockSetName:
                {
                    i += strm.db.handleDataPacketLockSetName(reinterpret_cast<const DataPacketLockSetName*>(&pDst[i]));
                    break;
                }
                case DataStreamType::LockTry:
                {
                    i += strm.db.handleDataPacketLockTry(reinterpret_cast<const DataPacketLockTry*>(&pDst[i]));
                    break;
                }
                case DataStreamType::LockState:
                {
                    i += strm.db.handleDataPacketLockState(reinterpret_cast<const DataPacketLockState*>(&pDst[i]));
                    break;
                }
                case DataStreamType::LockCount:
                {
                    i += strm.db.handleDataPacketLockCount(reinterpret_cast<const DataPacketLockCount*>(&pDst[i]));
                    break;
                }
                case DataStreamType::MemAlloc:
                {
                    i += strm.db.handleDataPacketMemAlloc(reinterpret_cast<const DataPacketMemAlloc*>(&pDst[i]));
                    break;
                }
                case DataStreamType::MemFree:
                {
                    i += strm.db.handleDataPacketMemFree(reinterpret_cast<const DataPacketMemFree*>(&pDst[i]));
                    break;
                }
                case DataStreamType::Message:
                {
                    i += strm.db.handleDataPacketMessage(reinterpret_cast<const DataPacketMessage*>(&pDst[i]));
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

    cmdInsertZone.prepare("INSERT INTO zones (threadID, startTick, endTick, packedSourceInfo, stackDepth, argData) VALUES(?,?,?,?,?,?)");
    cmdInsertString.prepare("INSERT INTO strings (Id, value) VALUES(?, ?)");
    cmdInsertTickInfo.prepare("INSERT INTO ticks (threadId, startTick, endTick, startNano, endNano) VALUES(?,?,?,?,?)");
    cmdInsertLock.prepare("INSERT INTO locks (Id) VALUES(?)");
    cmdInsertLockTry.prepare("INSERT INTO lockTry (lockId, threadId, startTick, endTick, result, depth, packedSourceInfo, argData) VALUES(?,?,?,?,?,?,?,?)");
    cmdInsertLockState.prepare("INSERT INTO lockStates (lockId, threadId, timeTicks, state, packedSourceInfo, argData) VALUES(?,?,?,?,?,?)");
    cmdInsertLockName.prepare("INSERT INTO lockNames (lockId, timeTicks, fmtStrIdx, argData) VALUES(?,?,?,?)");
    cmdInsertThreadName.prepare("INSERT INTO threadNames (threadId, timeTicks, fmtStrIdx, argData) VALUES(?,?,?,?)");
    cmdInsertMeta.prepare("INSERT INTO meta (name, value) VALUES(?,?)");
    cmdInsertMemory.prepare("INSERT INTO memory (allocId, size, threadId, timeTicks, operation, packedSourceInfo, argData) VALUES(?,?,?,?,?,?,?)");
    cmdInsertMessage.prepare("INSERT INTO messages (timeTicks, type, fmtStrIdx, argData) VALUES(?,?,?,?)");
    return true;
}

bool TraceDB::openDB(core::Path<char>& path)
{
    // TODO: drop write when not needed
    if (!con.connect(path.c_str(), sql::OpenFlag::WRITE)) {
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
    )");

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "Failed to creat indexes");
        return false;
    }

    trans.commit();
    return true;
}


bool TraceDB::createTables(void)
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
    "fmtStrIdx"         INTEGER NOT NULL,
    "argData"	        BLOB,
    PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "strings" (
	"Id"	            INTEGER,
	"value"	            TEXT NOT NULL UNIQUE,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "zoneInfo" (
    "id"                INTEGER,
    "totalTime"         INTEGER NOT NULL,
    "childTime"         INTEGER NOT NULL,
    "hitCount"          INTEGER NOT NULL,
    PRIMARY KEY("id")
);

CREATE TABLE IF NOT EXISTS "zones" (
    "id"	            INTEGER,
    "zoneId"	        INTEGER,
    "threadId"	        INTEGER NOT NULL,
    "startTick"	        INTEGER NOT NULL,
    "endTick"	        INTEGER NOT NULL,
    "packedSourceInfo"	INTEGER NOT NULL,
    "stackDepth"	    INTEGER NOT NULL,
    "argData"	        BLOB,
    PRIMARY KEY("id"),
    FOREIGN KEY("zoneId") REFERENCES "zoneInfo"("Id")
);

CREATE TABLE IF NOT EXISTS "locks" (
	"Id"	            INTEGER,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "lockNames" (
    "Id"                INTEGER,
    "lockId"            INTEGER NOT NULL,
    "timeTicks"         INTEGER NOT NULL,
    "fmtStrIdx"         INTEGER NOT NULL,
    "argData"	        BLOB,
    PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "lockTry" (
	"Id"	            INTEGER,
	"lockId"	        INTEGER NOT NULL,
    "threadId"	        INTEGER NOT NULL,
	"startTick"	        INTEGER NOT NULL,
	"endTick"	        INTEGER NOT NULL,
    "depth"	            INTEGER NOT NULL,
    "result"	        INTEGER NOT NULL,
	"packedSourceInfo"	INTEGER NOT NULL,
    "argData"	        BLOB,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "lockStates" (
	"Id"	            INTEGER,
	"lockId"	        INTEGER NOT NULL,
	"threadId"	        INTEGER NOT NULL,
	"timeTicks"	        INTEGER NOT NULL,
	"state"	            INTEGER NOT NULL,
    "packedSourceInfo"	INTEGER NOT NULL,
    "argData"	        BLOB,
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
    "argData"	        BLOB,
	PRIMARY KEY("Id")
);

CREATE TABLE "messages" (
	"Id"	            INTEGER,
	"type"	            INTEGER NOT NULL,
	"timeTicks"	        INTEGER NOT NULL,
	"fmtStrIdx"	        INTEGER NOT NULL,
    "argData"	        BLOB,
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

int32_t TraceDB::handleDataPacketTickInfo(const DataPacketTickInfo* pData)
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

int32_t TraceDB::handleDataPacketStringTableAdd(const DataPacketStringTableAdd* pData)
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
    return sizeof(std::remove_pointer_t<decltype(pData)>);
}

int32_t TraceDB::handleDataPacketZone(const DataPacketZone* pData)
{
    const int32_t argDataSize = pData->argDataSize;
    const int32_t totalSize = sizeof(*pData) + argDataSize;
    auto* pArgData = reinterpret_cast<const void*>(pData + 1);

    PackedSourceInfo info;
    info.raw.lineNo = pData->lineNo;
    info.raw.idxFunction = pData->strIdxFunction;
    info.raw.idxFile = pData->strIdxFile;
    info.raw.idxFmt = pData->strIdxFmt;

    uint64_t sourceInfo = info.packed;

    auto& cmd = cmdInsertZone;
    cmd.bind(1, static_cast<int32_t>(pData->threadID));
    cmd.bind(2, static_cast<int64_t>(pData->start));
    cmd.bind(3, static_cast<int64_t>(pData->end));
    cmd.bind(4, static_cast<int64_t>(sourceInfo));
    cmd.bind(5, pData->stackDepth);
    cmd.bind(6, argDataSize ? pArgData : nullptr, argDataSize);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return totalSize;
}

int32_t TraceDB::handleDataPacketLockTry(const DataPacketLockTry* pData)
{
    const int32_t argDataSize = pData->argDataSize;
    const int32_t totalSize = sizeof(*pData) + argDataSize;
    auto* pArgData = reinterpret_cast<const void*>(pData + 1);

    insertLockIfMissing(pData->lockHandle);

    PackedSourceInfo info;
    info.raw.lineNo = pData->lineNo;
    info.raw.idxFunction = pData->strIdxFunction;
    info.raw.idxFile = pData->strIdxFile;
    info.raw.idxFmt = pData->strIdxFmt;

    auto& cmd = cmdInsertLockTry;
    cmd.bind(1, static_cast<int64_t>(pData->lockHandle));
    cmd.bind(2, static_cast<int32_t>(pData->threadID));
    cmd.bind(3, static_cast<int64_t>(pData->start));
    cmd.bind(4, static_cast<int64_t>(pData->end));
    cmd.bind(5, static_cast<int32_t>(pData->result));
    cmd.bind(6, static_cast<int64_t>(pData->depth));
    cmd.bind(7, static_cast<int64_t>(info.packed));
    cmd.bind(8, argDataSize ? pArgData : nullptr, argDataSize);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return totalSize;
}

int32_t TraceDB::handleDataPacketLockState(const DataPacketLockState* pData)
{
    const int32_t argDataSize = pData->argDataSize;
    const int32_t totalSize = sizeof(*pData) + argDataSize;
    auto* pArgData = reinterpret_cast<const void*>(pData + 1);

    insertLockIfMissing(pData->lockHandle);

    PackedSourceInfo info;
    info.raw.lineNo = pData->lineNo;
    info.raw.idxFunction = pData->strIdxFunction;
    info.raw.idxFile = pData->strIdxFile;
    info.raw.idxFmt = pData->strIdxFmt;

    auto& cmd = cmdInsertLockState;
    cmd.bind(1, static_cast<int64_t>(pData->lockHandle));
    cmd.bind(2, static_cast<int32_t>(pData->threadID));
    cmd.bind(3, static_cast<int64_t>(pData->time));
    cmd.bind(4, static_cast<int64_t>(pData->state));
    cmd.bind(5, static_cast<int64_t>(info.packed));
    cmd.bind(6, argDataSize ? pArgData : nullptr, argDataSize);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return totalSize;
}

int32_t TraceDB::handleDataPacketLockSetName(const DataPacketLockSetName* pData)
{
    const int32_t argDataSize = pData->argDataSize;
    const int32_t totalSize = sizeof(*pData) + argDataSize;
    auto* pArgData = reinterpret_cast<const void*>(pData + 1);

    insertLockIfMissing(pData->lockHandle);

    auto& cmd = cmdInsertLockName;
    cmd.bind(1, static_cast<int64_t>(pData->lockHandle));
    cmd.bind(2, static_cast<int64_t>(pData->time)); 
    cmd.bind(3, static_cast<int32_t>(pData->strIdxFmt));
    cmd.bind(4, argDataSize ? pArgData : nullptr, argDataSize);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return totalSize;
}

int32_t TraceDB::handleDataPacketThreadSetName(const DataPacketThreadSetName* pData)
{
    const int32_t argDataSize = pData->argDataSize;
    const int32_t totalSize = sizeof(*pData) + argDataSize;
    auto* pArgData = reinterpret_cast<const void*>(pData + 1);

    auto& cmd = cmdInsertThreadName;
    cmd.bind(1, static_cast<int32_t>(pData->threadID));
    cmd.bind(2, static_cast<int64_t>(pData->time));
    cmd.bind(3, static_cast<int32_t>(pData->strIdxFmt));
    cmd.bind(4, argDataSize ? pArgData : nullptr, argDataSize);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return totalSize;
}

int32_t TraceDB::handleDataPacketLockCount(const DataPacketLockCount* pData)
{
    X_UNUSED(pData);
    // not sure how best to store this just yet.

    return sizeof(*pData);
}

int32_t TraceDB::handleDataPacketMemAlloc(const DataPacketMemAlloc* pData)
{
    const int32_t argDataSize = pData->argDataSize;
    const int32_t totalSize = sizeof(*pData) + argDataSize;
    auto* pArgData = reinterpret_cast<const void*>(pData + 1);

    PackedSourceInfo info;
    info.raw.lineNo = pData->lineNo;
    info.raw.idxFunction = pData->strIdxFunction;
    info.raw.idxFile = pData->strIdxFile;
    info.raw.idxFmt = pData->strIdxFmt;

    auto& cmd = cmdInsertMemory;
    cmd.bind(1, static_cast<int32_t>(pData->ptr));
    cmd.bind(2, static_cast<int32_t>(pData->size));
    cmd.bind(3, static_cast<int32_t>(pData->threadID));
    cmd.bind(4, static_cast<int64_t>(pData->time));
    cmd.bind(5, static_cast<int32_t>(MemOp::Alloc));
    cmd.bind(6, static_cast<int64_t>(info.packed));
    cmd.bind(7, argDataSize ? pArgData : nullptr, argDataSize);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return totalSize;
}

int32_t TraceDB::handleDataPacketMemFree(const DataPacketMemFree* pData)
{
    PackedSourceInfo info;
    info.raw.lineNo = pData->lineNo;
    info.raw.idxFunction = pData->strIdxFunction;
    info.raw.idxFile = pData->strIdxFile;
    info.raw.idxFmt = 0;

    auto& cmd = cmdInsertMemory;
    cmd.bind(1, static_cast<int32_t>(pData->ptr));
    cmd.bind(2, -1);
    cmd.bind(3, static_cast<int32_t>(pData->threadID));
    cmd.bind(4, static_cast<int64_t>(pData->time));
    cmd.bind(5, static_cast<int32_t>(MemOp::Free));
    cmd.bind(6, static_cast<int64_t>(info.packed));

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return sizeof(*pData);
}

int32_t TraceDB::handleDataPacketMessage(const DataPacketMessage* pData)
{
    const int32_t argDataSize = pData->argDataSize;
    const int32_t totalSize = sizeof(*pData) + argDataSize;
    auto* pArgData = reinterpret_cast<const void*>(pData + 1);

    auto& cmd = cmdInsertMessage;
    cmd.bind(1, static_cast<int32_t>(pData->time));
    cmd.bind(2, static_cast<int32_t>(pData->logType));
    cmd.bind(3, static_cast<int32_t>(pData->strIdxFmt));
    cmd.bind(4, argDataSize ? pArgData : nullptr, argDataSize);

    auto res = cmd.execute();
    if (res != sql::Result::OK) {
        X_ERROR("TelemSrv", "insert err(%i): \"%s\"", res, con.errorMsg());
    }

    cmd.reset();
    return totalSize;
}

int32_t TraceDB::handleDataPacketCallStack(const DataPacketCallStack* pData)
{
    X_UNUSED(pData);

    // TODO: ...

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
        loaded &= getMetaUInt64(db, "tickPerMs", trace.ticksPerMs);

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

        // TEMP: add index if needed
        if (client.type == ClientType::TraceStream) {
            client.traceStrm.db.createIndexes();
        }

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

        case PacketType::ReqTraceZoneSegment:
            return handleReqTraceZoneSegment(client, pData);
        case PacketType::ReqTraceLocks:
            return handleReqTraceLocks(client, pData);
        case PacketType::ReqTraceStrings:
            return handleReqTraceStrings(client, pData);
        case PacketType::ReqTraceThreadNames:
            return handleReqTraceThreadNames(client, pData);
        case PacketType::ReqTraceLockNames:
            return handleReqTraceLockNames(client, pData);
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
    setMeta &= strm.db.setMeta<int64_t>("tickPerMs", static_cast<int64_t>(trace.ticksPerMs));

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

    X_ASSERT(client.type == ClientType::Unknown, "Client type already set")(client.type);
    client.type = ClientType::TraceStream;
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

    X_ASSERT(client.type == ClientType::Unknown, "Client type already set")(client.type);
    client.type = ClientType::Viewer;
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
                if (getStats(db, stats))
                {
                    QueryTraceInfoResp resp;
                    resp.type = PacketType::QueryTraceInfoResp;
                    resp.dataSize = sizeof(resp);
                    resp.guid = pHdr->guid;
                    resp.stats = stats;

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
    for (size_t i = 0; i < client.traces.size();i++)
    {
        auto& trace = client.traces[i];
        if (trace.pTrace->guid == pHdr->guid)
        {
            X_WARNING("TelemSrv", "Client opened a trace they already have open");

            if (!getStats(trace.db.con, otr.stats))
            {
                X_ERROR("TelemSrv", "Failed to get stats for openDb request");
                return true;
            }

            otr.handle = safe_static_cast<int8_t>(i);
            otr.ticksPerMicro = trace.pTrace->ticksPerMicro;
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
                    if (ts.db.openDB(trace.dbPath)) 
                    {
                        if (!getStats(ts.db.con, otr.stats))
                        {
                            X_ERROR("TelemSrv", "Failed to get stats for openDb request");
                            return true;
                        }

                        auto id = client.traces.size();
                        client.traces.emplace_back(std::move(ts));

                        otr.handle = safe_static_cast<int8_t>(id);
                        otr.ticksPerMicro = trace.ticksPerMicro;
                        sendDataToClient(client, &otr, sizeof(otr));
                        return true;
                    }
                }
            }
        }
    }

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

X_INLINE int32_t getCompressionBufferSize(ClientConnection& client)
{
    return client.cmpBufEnd - client.cmpBufBegin;
}

X_INLINE int32_t getCompressionBufferSpace(ClientConnection& client)
{
    const int32_t space = COMPRESSION_MAX_INPUT_SIZE - (client.cmpBufEnd - client.cmpBufBegin);

    return space;
}

void addToCompressionBuffer(ClientConnection& client, const void* pData, int32_t len)
{
#if X_DEBUG
    if (len > COMPRESSION_MAX_INPUT_SIZE) {
        ::DebugBreak();
    }
#endif // X_DEBUG

    // can we fit this data?
    const int32_t space = getCompressionBufferSpace(client);
    if (space < len) {
        flushCompressionBuffer(client);
    }

    memcpy(&client.cmpRingBuf[client.cmpBufEnd], pData, len);
    client.cmpBufEnd += len;
}

template<typename T>
T* addToCompressionBufferT(ClientConnection& client)
{
#if X_DEBUG
    if constexpr (sizeof(T) > COMPRESSION_MAX_INPUT_SIZE) {
        ::DebugBreak();
    }
#endif // X_DEBUG

    // can we fit this data?
    const int32_t space = getCompressionBufferSpace(client);
    if (space < sizeof(T)) {
        flushCompressionBuffer(client);
    }

    static_assert(std::is_trivially_copyable_v<T>, "T is not trivially copyable");

    T* pPtr = reinterpret_cast<T*>(&client.cmpRingBuf[client.cmpBufEnd]);
    client.cmpBufEnd += sizeof(T);
    return pPtr;
}


X_DISABLE_WARNING(4701) // potentially uninitialized local variable 'tick' used


bool Server::handleReqTraceZoneSegment(ClientConnection& client, uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceZoneSegment*>(pData);
    if (pHdr->type != PacketType::ReqTraceZoneSegment) {
        X_ASSERT_UNREACHABLE();
    }

    // MEOW
    // load me the ticks!
    int32_t handle = pHdr->handle;
    if (handle < 0 || handle >= static_cast<int32_t>(client.traces.size())) {
        return false;
    }

    auto& ts = client.traces[pHdr->handle];
    DataPacketTickInfo startTick;
    DataPacketTickInfo endTick;

    {
        auto* pTickHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespTicks>(client);
        pTickHdr->type = DataStreamTypeViewer::TraceZoneSegmentTicks;
        pTickHdr->num = 0;
        pTickHdr->handle = pHdr->handle;

        auto begin = ts.pTrace->nanoToTicks(pHdr->startNano);
        auto end = ts.pTrace->nanoToTicks(pHdr->endNano);

        sql::SqlLiteQuery qry(ts.db.con, "SELECT threadId, startTick, endTick, startNano, endNano FROM ticks WHERE startTick >= ? AND startTick < ?");
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

            if (getCompressionBufferSpace(client) < sizeof(tick))
            {
                // flush etc and add new block header.
                pTickHdr->num = numTicks;
                flushCompressionBuffer(client);

                pTickHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespTicks>(client);
                pTickHdr->type = DataStreamTypeViewer::TraceZoneSegmentTicks;
                pTickHdr->num = 0;
                pTickHdr->handle = pHdr->handle;

                numTicks = 0;
            }

            addToCompressionBuffer(client, &tick, sizeof(tick));

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
        flushCompressionBuffer(client);
    }


    auto start = startTick.start;
    auto end = endTick.end;

    {

        // so on the client i need to handle overlapping zones.
        // if we just take every zone that starts in the tick and draw it should be okay even if it overlaps.
        // we should still be able to pick it.
        auto* pZonesHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespZones>(client);
        pZonesHdr->type = DataStreamTypeViewer::TraceZoneSegmentZones;
        pZonesHdr->num = 0;
        pZonesHdr->handle = pHdr->handle;

        int32_t numZones = 0;

        sql::SqlLiteQuery qry(ts.db.con, "SELECT threadId, startTick, endTick, stackDepth, packedSourceInfo FROM zones WHERE startTick >= ? AND startTick < ?");
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
            zone.stackDepth = static_cast<tt_int8>(row.get<int32_t>(3));

            TraceDB::PackedSourceInfo info;
            info.packed = static_cast<uint64_t>(row.get<int64_t>(4));

            zone.lineNo = info.raw.lineNo;
            zone.strIdxFunction = info.raw.idxFunction;
            zone.strIdxFile = info.raw.idxFile;
            zone.strIdxFmt = info.raw.idxFmt;

            if (getCompressionBufferSpace(client) < sizeof(zone))
            {
                // flush etc and add new block header.
                pZonesHdr->num = numZones;
                flushCompressionBuffer(client);

                pZonesHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespZones>(client);
                pZonesHdr->type = DataStreamTypeViewer::TraceZoneSegmentZones;
                pZonesHdr->num = 0;
                pZonesHdr->handle = pHdr->handle;

                numZones = 0;
            }

            addToCompressionBuffer(client, &zone, sizeof(zone));

            ++numZones;
        }

        if (numZones) {
            pZonesHdr->num = numZones;
            flushCompressionBuffer(client);
        }
    }

    {
        sql::SqlLiteQuery qry(ts.db.con, "SELECT lockId, threadId, startTick, endTick, result, depth, packedSourceInfo FROM lockTry WHERE startTick >= ? AND startTick < ?");
        qry.bind(1, static_cast<int64_t>(start));
        qry.bind(2, static_cast<int64_t>(end));

        auto* pLockTryHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespLockTry>(client);
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
            lockTry.result = static_cast<TtLockResult>(row.get<int32_t>(4));
            lockTry.depth = static_cast<uint8_t>(row.get<int32_t>(5) & 0xFFFF);

            TraceDB::PackedSourceInfo info;
            info.packed = static_cast<uint64_t>(row.get<int64_t>(6));

            lockTry.lineNo = info.raw.lineNo;
            lockTry.strIdxFunction = info.raw.idxFunction;
            lockTry.strIdxFile = info.raw.idxFile;
            lockTry.strIdxFmt = info.raw.idxFmt;

            if (getCompressionBufferSpace(client) < sizeof(lockTry))
            {
                // flush etc and add new block header.
                pLockTryHdr->num = numLockTry;
                flushCompressionBuffer(client);

                pLockTryHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespLockTry>(client);
                pLockTryHdr->type = DataStreamTypeViewer::TraceZoneSegmentLockTry;
                pLockTryHdr->num = 0;
                pLockTryHdr->handle = pHdr->handle;

                numLockTry = 0;
            }

            addToCompressionBuffer(client, &lockTry, sizeof(lockTry));

            ++numLockTry;
        }

        if (numLockTry) {
            pLockTryHdr->num = numLockTry;
            flushCompressionBuffer(client);
        }
    }

    {
        // lockStates?
        sql::SqlLiteQuery qry(ts.db.con, "SELECT lockId, threadId, timeTicks, state, packedSourceInfo FROM lockStates WHERE timeTicks >= ? AND timeTicks < ?");
        qry.bind(1, static_cast<int64_t>(start));
        qry.bind(2, static_cast<int64_t>(end));

        auto* pLockStateHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespLockStates>(client);
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
            lockState.state = static_cast<TtLockState>(row.get<int32_t>(3));

            TraceDB::PackedSourceInfo info;
            info.packed = static_cast<uint64_t>(row.get<int64_t>(4));

            lockState.lineNo = info.raw.lineNo;
            lockState.strIdxFunction = info.raw.idxFunction;
            lockState.strIdxFile = info.raw.idxFile;

            if (getCompressionBufferSpace(client) < sizeof(lockState))
            {
                // flush etc and add new block header.
                pLockStateHdr->num = numLockState;
                flushCompressionBuffer(client);

                pLockStateHdr = addToCompressionBufferT<ReqTraceZoneSegmentRespLockStates>(client);
                pLockStateHdr->type = DataStreamTypeViewer::TraceZoneSegmentLockStates;
                pLockStateHdr->num = 0;
                pLockStateHdr->handle = pHdr->handle;

                numLockState = 0;
            }

            addToCompressionBuffer(client, &lockState, sizeof(lockState));

            ++numLockState;
        }

        if (numLockState) {
            pLockStateHdr->num = numLockState;
            flushCompressionBuffer(client);
        }
    }

    X_ASSERT(getCompressionBufferSize(client) == 0, "Compression buffer is not empty")();
    return true;
}

X_ENABLE_WARNING(4701)

bool Server::handleReqTraceLocks(ClientConnection& client, uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceLocks*>(pData);
    if (pHdr->type != PacketType::ReqTraceLocks) {
        X_ASSERT_UNREACHABLE();
    }

    int32_t handle = pHdr->handle;
    if (handle < 0 || handle >= static_cast<int32_t>(client.traces.size())) {
        return false;
    }

    auto& ts = client.traces[pHdr->handle];

    auto* pLocksHdr = addToCompressionBufferT<ReqTraceLocksResp>(client);
    pLocksHdr->type = DataStreamTypeViewer::TraceLocks;
    pLocksHdr->handle = pHdr->handle;

    int32_t num = 0;

    sql::SqlLiteQuery qry(ts.db.con, "SELECT id FROM locks");

    auto it = qry.begin();
    for (; it != qry.end(); ++it) {
        auto row = *it;

        TraceLockData tld;
        tld.id = static_cast<uint64_t>(row.get<int64_t>(0));

        if (getCompressionBufferSpace(client) < sizeof(tld)) {
            pLocksHdr->num = num;
            num = 0;

            flushCompressionBuffer(client);

            pLocksHdr = addToCompressionBufferT<ReqTraceLocksResp>(client);
            pLocksHdr->type = DataStreamTypeViewer::TraceLocks;
            pLocksHdr->handle = pHdr->handle;
        }

        addToCompressionBuffer(client, &tld, sizeof(tld));

        num++;
    }

    if (num) {
        pLocksHdr->num = num;
        flushCompressionBuffer(client);
    }
    return true;
}

bool Server::handleReqTraceStrings(ClientConnection& client, uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceStrings*>(pData);
    if (pHdr->type != PacketType::ReqTraceStrings) {
        X_ASSERT_UNREACHABLE();
    }

    int32_t handle = pHdr->handle;
    if (handle < 0 || handle >= static_cast<int32_t>(client.traces.size())) {
        return false;
    }

    auto& ts = client.traces[pHdr->handle];

    ReqTraceStringsRespInfo info;
    info.type = DataStreamTypeViewer::TraceStringsInfo;
    info.handle = pHdr->handle;

    {
        // lets just count it will be cheap.    
        // TODO: pretty easy to calculate this in ingest if it becomes slow.
        sql::SqlLiteQuery qry(ts.db.con, "SELECT COUNT(_rowid_), SUM(LENGTH(value)), MIN(Id), MAX(Id) FROM strings");
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

    addToCompressionBuffer(client, &info, sizeof(info));

    // TODO: hack until client can handle multiple per packet
    flushCompressionBuffer(client);

    auto* pStringsHdr = addToCompressionBufferT<ReqTraceStringsResp>(client);
    core::zero_this(pStringsHdr);
    pStringsHdr->type = DataStreamTypeViewer::TraceStrings;
    pStringsHdr->handle = pHdr->handle;

    int32_t num = 0;

    if (info.num > 0)
    {
        sql::SqlLiteQuery qry(ts.db.con, "SELECT id, value FROM strings");

        auto it = qry.begin();
        for (; it != qry.end(); ++it) {
            auto row = *it;

            int32_t id = row.get<int32_t>(0);
            int32_t strLen = row.columnBytes(1);
            const char* pStr = row.get<const char*>(1);

            TraceStringHdr strHdr;
            strHdr.id = static_cast<uint16_t>(id);
            strHdr.length = static_cast<uint16_t>(strLen);

            if (getCompressionBufferSpace(client) < sizeof(strHdr) + strLen)
            {
                pStringsHdr->num = num;
                num = 0;

                flushCompressionBuffer(client);

                pStringsHdr = addToCompressionBufferT<ReqTraceStringsResp>(client);
                core::zero_this(pStringsHdr);
                pStringsHdr->type = DataStreamTypeViewer::TraceStrings;
                pStringsHdr->handle = pHdr->handle;
            }

            addToCompressionBuffer(client, &strHdr, sizeof(strHdr));
            addToCompressionBuffer(client, pStr, strLen);

            num++;
        }

        if (num) {
            pStringsHdr->num = num;
            flushCompressionBuffer(client);
        }
    }

    X_ASSERT(getCompressionBufferSize(client) == 0, "Compression buffer is not empty")();
    return true;
}

bool Server::handleReqTraceThreadNames(ClientConnection& client, uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceThreadNames*>(pData);
    if (pHdr->type != PacketType::ReqTraceThreadNames) {
        X_ASSERT_UNREACHABLE();
    }

    int32_t handle = pHdr->handle;
    if (handle < 0 || handle >= static_cast<int32_t>(client.traces.size())) {
        return false;
    }

    auto& ts = client.traces[pHdr->handle];

    auto* pNamesHdr = addToCompressionBufferT<ReqTraceThreadNamesResp>(client);
    core::zero_this(pNamesHdr);
    pNamesHdr->type = DataStreamTypeViewer::TraceThreadNames;
    pNamesHdr->handle = pHdr->handle;

    int32_t num = 0;

    sql::SqlLiteQuery qry(ts.db.con, "SELECT threadId, timeTicks, fmtStrId FROM threadNames");

    auto it = qry.begin();
    for (; it != qry.end(); ++it) {
        auto row = *it;

        TraceThreadNameData tnd;
        tnd.threadId = row.get<int32_t>(0);
        tnd.timeTicks = row.get<int64_t>(1);
        tnd.strIdx = safe_static_cast<uint16_t>(row.get<int32_t>(2));
              
        if (getCompressionBufferSpace(client) < sizeof(tnd)) {
            pNamesHdr->num = num;
            num = 0;

            flushCompressionBuffer(client);

            pNamesHdr = addToCompressionBufferT<ReqTraceThreadNamesResp>(client);
            core::zero_this(pNamesHdr);
            pNamesHdr->type = DataStreamTypeViewer::TraceThreadNames;
            pNamesHdr->handle = pHdr->handle;
        }

        addToCompressionBuffer(client, &tnd, sizeof(tnd));

        num++;
    }

    if (num) {
        pNamesHdr->num = num;
        flushCompressionBuffer(client);
    }
    
    return true;
}

bool Server::handleReqTraceLockNames(ClientConnection& client, uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const ReqTraceLockNames*>(pData);
    if (pHdr->type != PacketType::ReqTraceLockNames) {
        X_ASSERT_UNREACHABLE();
    }

    int32_t handle = pHdr->handle;
    if (handle < 0 || handle >= static_cast<int32_t>(client.traces.size())) {
        return false;
    }

    auto& ts = client.traces[pHdr->handle];

    auto* pNamesHdr = addToCompressionBufferT<ReqTraceLockNamesResp>(client);
    core::zero_this(pNamesHdr);
    pNamesHdr->type = DataStreamTypeViewer::TraceLockNames;
    pNamesHdr->handle = pHdr->handle;

    int32_t num = 0;

    sql::SqlLiteQuery qry(ts.db.con, "SELECT lockId, timeTicks, fmtStrId FROM lockNames");

    auto it = qry.begin();
    for (; it != qry.end(); ++it) {
        auto row = *it;

        TraceLockNameData lnd;
        lnd.lockId = static_cast<uint64_t>(row.get<int64_t>(0));
        lnd.timeTicks = row.get<int64_t>(1);
        lnd.strIdx = safe_static_cast<uint16_t>(row.get<int32_t>(2));

        if (getCompressionBufferSpace(client) < sizeof(lnd)) {
            pNamesHdr->num = num;
            num = 0;

            flushCompressionBuffer(client);

            pNamesHdr = addToCompressionBufferT<ReqTraceLockNamesResp>(client);
            core::zero_this(pNamesHdr);
            pNamesHdr->type = DataStreamTypeViewer::TraceLockNames;
            pNamesHdr->handle = pHdr->handle;
        }

        addToCompressionBuffer(client, &lnd, sizeof(lnd));

        num++;
    }

    if (num) {
        pNamesHdr->num = num;
        flushCompressionBuffer(client);
    }

    return true;
}



X_NAMESPACE_END