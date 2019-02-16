#include "stdafx.h"
#include "TelemetryServerLib.h"

#include <Compression/LZ4.h>
#include <Time/DateTimeStamp.h>

#include <IFileSys.h>

#include <../SqLite/SqlLib.h>
#include <../TelemetryCommon/TelemetryCommonLib.h>
// #include <winsock2.h>

X_LINK_LIB("engine_TelemetryCommonLib.lib");

namespace
{
    const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);

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

    struct TraceDB
    {
        TraceDB() :
            cmdInsertZone(con),
            cmdInsertString(con),
            cmdInsertTickInfo(con),
            cmdInsertLock(con),
            cmdInsertLockTry(con),
            cmdInsertLockState(con),
            cmdInsertThreadName(con),
            cmdInsertLockName(con),
            cmdInsertMeta(con),
            cmdInsertMemory(con)
        {
        }

        sql::SqlLiteDb con;

        sql::SqlLiteCmd cmdInsertZone;
        sql::SqlLiteCmd cmdInsertString;
        sql::SqlLiteCmd cmdInsertTickInfo;
        sql::SqlLiteCmd cmdInsertLock;
        sql::SqlLiteCmd cmdInsertLockTry;
        sql::SqlLiteCmd cmdInsertLockState;
        sql::SqlLiteCmd cmdInsertThreadName;
        sql::SqlLiteCmd cmdInsertLockName;
        sql::SqlLiteCmd cmdInsertMeta;
        sql::SqlLiteCmd cmdInsertMemory;
    };


    bool createTables(TraceDB& db)
    {

        if (!db.con.execute(R"(

CREATE TABLE IF NOT EXISTS "meta" (
	"Id"	        INTEGER,
	"name"	        TEXT NOT NULL UNIQUE,
	"value"	        TEXT NOT NULL,
	PRIMARY KEY("Id")
);

CREATE TABLE IF NOT EXISTS "ticks" (
	"Id"	        INTEGER,
	"threadId"	    INTEGER NOT NULL,
	"time"	        INTEGER NOT NULL,
	"timeMicro"	    INTEGER NOT NULL,
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
    "start"	        INTEGER NOT NULL,
    "end"	        INTEGER NOT NULL,
    "sourceInfoIdx"	INTEGER NOT NULL,
    "stackDepth"	INTEGER NOT NULL,
    PRIMARY KEY("id")
);

CREATE TABLE IF NOT EXISTS "locks" (
	"Id"	        INTEGER,
	"handle"	    INTEGER NOT NULL,
    "name"	        TEXT NOT NULL UNIQUE,
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

    bool createDB(TraceDB& db, core::Path<char>& path)
    {
        if (!db.con.connect(path.c_str(), sql::OpenFlag::CREATE | sql::OpenFlag::WRITE)) {
            return false;
        }

        if (!db.con.execute(R"(
                PRAGMA synchronous = OFF;
                PRAGMA page_size = 4096;
                PRAGMA cache_size = -4000;
                PRAGMA journal_mode = MEMORY;
                PRAGMA foreign_keys = ON;
            )")) {
            return false;
        }

        // create all the tables.
        if (!createTables(db)) {
            return false;
        }


        db.cmdInsertZone.prepare("INSERT INTO zones (threadID, start, end, sourceInfoIdx, stackDepth) VALUES(?,?,?,?,?)");
        db.cmdInsertString.prepare("INSERT INTO strings (value) VALUES(?)");
        db.cmdInsertTickInfo.prepare("INSERT INTO ticks (threadId, time, timeMicro) VALUES(?,?,?)");
        db.cmdInsertLock.prepare("INSERT INTO locks (handle, name) VALUES(?,?)");
        db.cmdInsertLockTry.prepare("INSERT INTO lockTry (lockId, threadId, start, end, descriptionStrId) VALUES(?,?,?,?,?)");
        db.cmdInsertLockState.prepare("INSERT INTO lockStates (lockId, threadId, time, state) VALUES(?,?,?,?)");
        db.cmdInsertLockName.prepare("INSERT INTO lockNames (lockId, time, nameStrId) VALUES(?,?,?)");
        db.cmdInsertThreadName.prepare("INSERT INTO threadNames (threadId, time, nameStrId) VALUES(?,?,?)");
        db.cmdInsertMeta.prepare("INSERT INTO meta (name, value) VALUES(?,?)");
        db.cmdInsertMemory.prepare("INSERT INTO memory (allocId, size, threadId, time, operation) VALUES(?,?,?,?,?)");

        return true;
    }


    bool setMeta(TraceDB& db, const char* pName, const char* pValue)
    {
        auto& cmd = db.cmdInsertMeta;
        cmd.reset();
        cmd.bind(1, pName);
        cmd.bind(2, pValue);

        sql::Result::Enum res = cmd.execute();
        if (res != sql::Result::OK) {
            return false;
        }
        
        return true;
    }


    struct Client
    {
        Client() {
            connected = false;
            core::zero_object(clientVer);
            cmpBufferOffset = 0;
            pFile = nullptr;
            socket = INV_SOCKET;

#if X_DEBUG
            core::zero_object(appName);
            core::zero_object(buildInfo);
            core::zero_object(cmdLine);
            core::zero_object(srcRingBuf);
#endif // X_DEBUG
        }

        bool connected;
        VersionInfo clientVer;

        // TODO: not need nullterm.
        char appName[MAX_STRING_LEN + 1];
        char buildInfo[MAX_STRING_LEN + 1];
        char cmdLine[MAX_CMDLINE_LEN + 1];

        platform::SOCKET socket;
        
        TraceDB db;

        int32_t cmpBufferOffset;
        int8_t srcRingBuf[COMPRESSION_RING_BUFFER_SIZE];

        core::Compression::LZ4StreamDecode lz4Stream;

        FILE* pFile;
    };

    bool readPacket(Client& client, char* pBuffer, int& bufLengthInOut)
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
                    X_ERROR("TelemSrv", "Client sent oversied packet of size %i...", static_cast<tt_int32>(pHdr->dataSize));
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

    void sendPacketToClient(Client& client, const void* pData, size_t len)
    {
        // send some data...
        int res = platform::send(client.socket, reinterpret_cast<const char*>(pData), static_cast<int>(len), 0);
        if (res == SOCKET_ERROR) {
            X_LOG0("TelemSrv", "send failed with error: %d", platform::WSAGetLastError());
            return;
        }
    }

    void sendConnectionRejected(Client& client, const char* pReason)
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
        sendPacketToClient(client, buf, datalen);
    }

    bool handleConnectionRequest(Client& client, uint8_t* pData)
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

        client.connected = true;
        client.clientVer = pConReq->clientVer;

        // now need to read the strings.
        core::zero_object(client.appName);
        core::zero_object(client.buildInfo);
        core::zero_object(client.cmdLine);

        auto* pStrData = reinterpret_cast<const uint8_t*>(pConReq + 1);
        memcpy(client.appName, pStrData, pConReq->appNameLen);
        pStrData += pConReq->appNameLen;
        memcpy(client.buildInfo, pStrData, pConReq->buildInfoLen);
        pStrData += pConReq->buildInfoLen;
        memcpy(client.cmdLine, pStrData, pConReq->cmdLineLen);

        // make a db for the client.
        core::Path<char> workingDir;
        if (!gEnv->pFileSys->getWorkingDirectory(workingDir)) {
            return false;
        }

        core::DateTimeStamp date = core::DateTimeStamp::getSystemDateTime();
        core::DateTimeStamp::Description dateStr;

        core::Path<char> dbPath(workingDir);
        // want a folder for each app.
        dbPath.ensureSlash();
        dbPath.append("traces");
        dbPath.ensureSlash();
        dbPath.append(client.appName);
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

        if (!createDB(client.db, dbPath)) {
            return false;
        }

        setMeta(client.db, "appName", client.appName);
        setMeta(client.db, "buildInfo", client.buildInfo);
        setMeta(client.db, "cmdLine", client.cmdLine);

        // Meow...
        X_LOG0("TelemSrv", "ConnectionAccepted:");
        X_LOG0("TelemSrv", "> AppName: %s", client.appName);
        X_LOG0("TelemSrv", "> BuildInfo: %s", client.buildInfo);
        X_LOG0("TelemSrv", "> CmdLine: %s", client.cmdLine);
        X_LOG0("TelemSrv", "> DB: %s", dbPath.c_str());

        // send a packet back!
        ConnectionRequestAcceptedHdr cra;
        core::zero_object(cra);
        cra.dataSize = sizeof(cra);
        cra.type = PacketType::ConnectionRequestAccepted;
        cra.serverVer = serverVer;

        sendPacketToClient(client, &cra, sizeof(cra));
        return true;
    }

    void handleDataPacketTickInfo(TraceDB& db, const DataPacketTickInfo* pData)
    {
        auto& cmd = db.cmdInsertTickInfo;
        cmd.bind(1, static_cast<int32_t>(pData->threadID));
        cmd.bind(2, static_cast<int64_t>(pData->ticks));
        cmd.bind(3, static_cast<int64_t>(pData->timeMicro));

        auto res = cmd.execute();
        if (res != sql::Result::OK) {
            X_ERROR("SqlDb", "insert err(%i): \"%s\"", res, db.con.errorMsg());
        }

        cmd.reset();
    }

    void handleDataPacketStringTableAdd(TraceDB& db, const DataPacketStringTableAdd* pData)
    {
        const char* pString = reinterpret_cast<const char*>(pData + 1);

        auto& cmd = db.cmdInsertString;
        cmd.bind(1, pString, static_cast<size_t>(pData->length));

        auto res = cmd.execute();
        if (res != sql::Result::OK) {
            X_ERROR("SqlDb", "insert err(%i): \"%s\"", res, db.con.errorMsg());
        }

        cmd.reset();
    }

    void handleDataPacketZone(TraceDB& db, const DataPacketZone* pData)
    {
        auto& cmd = db.cmdInsertZone;
        cmd.bind(1, static_cast<int32_t>(pData->threadID));
        cmd.bind(2, static_cast<int64_t>(pData->start));
        cmd.bind(3, static_cast<int64_t>(pData->end));
        cmd.bind(4, -1);
        cmd.bind(5, pData->stackDepth);

        auto res = cmd.execute();
        if (res != sql::Result::OK) {
            X_ERROR("SqlDb", "insert err(%i): \"%s\"", res, db.con.errorMsg());
        }

        cmd.reset();
    }

    void handleDataPacketLockTry(TraceDB& db, const DataPacketLockTry* pData)
    {
        int32_t lockId = -1; // TODO

        auto& cmd = db.cmdInsertLockTry;
        cmd.bind(1, lockId);
        cmd.bind(2, static_cast<int32_t>(pData->threadID));
        cmd.bind(3, static_cast<int64_t>(pData->start));
        cmd.bind(4, static_cast<int64_t>(pData->end));
        cmd.bind(5, static_cast<int32_t>(pData->strIdxDescrption.index));

        auto res = cmd.execute();
        if (res != sql::Result::OK) {
            X_ERROR("SqlDb", "insert err(%i): \"%s\"", res, db.con.errorMsg());
        }

        cmd.reset();
    }

    void handleDataPacketLockState(TraceDB& db, const DataPacketLockState* pData)
    {
        int32_t lockId = -1; // TODO

        auto& cmd = db.cmdInsertLockState;
        cmd.bind(1, lockId);
        cmd.bind(2, static_cast<int32_t>(pData->threadID));
        cmd.bind(3, static_cast<int64_t>(pData->time));
        cmd.bind(4, static_cast<int64_t>(pData->state));

        auto res = cmd.execute();
        if (res != sql::Result::OK) {
            X_ERROR("SqlDb", "insert err(%i): \"%s\"", res, db.con.errorMsg());
        }

        cmd.reset();
    }

    void handleDataPacketLockSetName(TraceDB& db, const DataPacketLockSetName* pData)
    {
        int32_t lockId = -1; // TODO

        auto& cmd = db.cmdInsertLockName;
        cmd.bind(1, lockId);
        cmd.bind(2, static_cast<int64_t>(pData->time));
        cmd.bind(3, static_cast<int32_t>(pData->strIdxName.index));

        auto res = cmd.execute();
        if (res != sql::Result::OK) {
            X_ERROR("SqlDb", "insert err(%i): \"%s\"", res, db.con.errorMsg());
        }

        cmd.reset();
    }

    void handleDataPacketThreadSetName(TraceDB& db, const DataPacketThreadSetName* pData)
    {
        auto& cmd = db.cmdInsertThreadName;
        cmd.bind(1, static_cast<int32_t>(pData->threadID));
        cmd.bind(2, static_cast<int64_t>(pData->time));
        cmd.bind(3, static_cast<int32_t>(pData->strIdxName.index));

        auto res = cmd.execute();
        if (res != sql::Result::OK) {
            X_ERROR("SqlDb", "insert err(%i): \"%s\"", res, db.con.errorMsg());
        }

        cmd.reset();
    }

    void handleDataPacketLockCount(TraceDB& db, const DataPacketLockCount* pData)
    {
        X_UNUSED(db, pData);
        // not sure how best to store this just yet.
    }

    // maybe drop this and just use -1?
    enum class MemOp
    {
        Alloc,
        Free
    };

    void handleDataPacketMemAlloc(TraceDB& db, const DataPacketMemAlloc* pData)
    {
        auto& cmd = db.cmdInsertMemory;
        cmd.bind(1, static_cast<int32_t>(pData->ptr));
        cmd.bind(2, static_cast<int32_t>(pData->size));
        cmd.bind(3, static_cast<int32_t>(pData->threadID));
        cmd.bind(4, static_cast<int64_t>(pData->time));
        cmd.bind(5, static_cast<int32_t>(MemOp::Alloc));

        auto res = cmd.execute();
        if (res != sql::Result::OK) {
            X_ERROR("SqlDb", "insert err(%i): \"%s\"", res, db.con.errorMsg());
        }

        cmd.reset();
    }

    void handleDataPacketMemFree(TraceDB& db, const DataPacketMemFree* pData)
    {
        auto& cmd = db.cmdInsertMemory;
        cmd.bind(1, static_cast<int32_t>(pData->ptr));
        cmd.bind(2, -1);
        cmd.bind(3, static_cast<int32_t>(pData->threadID));
        cmd.bind(4, static_cast<int64_t>(pData->time));
        cmd.bind(5, static_cast<int32_t>(MemOp::Free));

        auto res = cmd.execute();
        if (res != sql::Result::OK) {
            X_ERROR("SqlDb", "insert err(%i): \"%s\"", res, db.con.errorMsg());
        }

        cmd.reset();
    }

    void handleDataPacketCallStack(TraceDB& db, const DataPacketCallStack* pData)
    {
        X_UNUSED(db, pData);

        // TODO: ...
    }


    bool handleDataSream(Client& client, uint8_t* pData)
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

        auto* pDst = &client.srcRingBuf[client.cmpBufferOffset];

        auto cmpLenOut = client.lz4Stream.decompressContinue(pHdr + 1, pDst, origLen);
        if (cmpLenOut != cmpLen) {
            // TODO: ..
            return false;
        }

        client.cmpBufferOffset += origLen;

        if (client.cmpBufferOffset >= (COMPRESSION_RING_BUFFER_SIZE - COMPRESSION_MAX_INPUT_SIZE)) {
            client.cmpBufferOffset = 0;
        }

        if (client.pFile) {
        //    fwrite(pDst, origLen, 1, client.pFile);
        }

        sql::SqlLiteTransaction trans(client.db.con);

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
                    handleDataPacketStringTableAdd(client.db, pStr);
                    i += sizeof(DataPacketStringTableAdd);
                    i += pStr->length;
                    break;
                }
                case DataStreamType::Zone:
                {
                    handleDataPacketZone(client.db, reinterpret_cast<const DataPacketZone*>(&pDst[i]));
                    i += sizeof(DataPacketZone);
                    break;
                }
                case DataStreamType::TickInfo:
                {
                    handleDataPacketTickInfo(client.db, reinterpret_cast<const DataPacketTickInfo*>(&pDst[i]));
                    i += sizeof(DataPacketTickInfo);
                    break;
                }
                case DataStreamType::ThreadSetName:
                {
                    handleDataPacketThreadSetName(client.db, reinterpret_cast<const DataPacketThreadSetName*>(&pDst[i]));
                    i += sizeof(DataPacketThreadSetName);
                    break;
                }
                case DataStreamType::CallStack:
                {
                    handleDataPacketCallStack(client.db, reinterpret_cast<const DataPacketCallStack*>(&pDst[i]));
                    i += sizeof(DataPacketCallStack);
                    break;
                }
                case DataStreamType::LockSetName:
                {
                    handleDataPacketLockSetName(client.db, reinterpret_cast<const DataPacketLockSetName*>(&pDst[i]));
                    i += sizeof(DataPacketLockSetName);
                    break;
                }
                case DataStreamType::LockTry:
                {
                    handleDataPacketLockTry(client.db, reinterpret_cast<const DataPacketLockTry*>(&pDst[i]));
                    i += sizeof(DataPacketLockTry);
                    break;
                }
                case DataStreamType::LockState:
                {
                    handleDataPacketLockState(client.db, reinterpret_cast<const DataPacketLockState*>(&pDst[i]));
                    i += sizeof(DataPacketLockState);
                    break;
                }
                case DataStreamType::LockCount:
                {
                    handleDataPacketLockCount(client.db, reinterpret_cast<const DataPacketLockCount*>(&pDst[i]));
                    i += sizeof(DataPacketLockCount);
                    break;
                }
                case DataStreamType::MemAlloc:
                {
                    handleDataPacketMemAlloc(client.db, reinterpret_cast<const DataPacketMemAlloc*>(&pDst[i]));
                    i += sizeof(DataPacketMemAlloc);
                    break;
                }
                case DataStreamType::MemFree:
                {
                    handleDataPacketMemFree(client.db, reinterpret_cast<const DataPacketMemFree*>(&pDst[i]));
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

    bool processPacket(Client& client, uint8_t* pData)
    {
        auto* pPacketHdr = reinterpret_cast<const PacketBase*>(pData);

        switch (pPacketHdr->type)
        {
            case PacketType::ConnectionRequest:
                return handleConnectionRequest(client, pData);
                break;
            case PacketType::DataStream:
                return handleDataSream(client, pData);
                break;
            default:
                X_ERROR("TelemSrv", "Unknown packet type %" PRIi32, static_cast<int>(pPacketHdr->type));
                return false;
        }
    }

    void handleClient(Client& client)
    {
        char recvbuf[MAX_PACKET_SIZE];

        while(1)
        {
            int recvbuflen = sizeof(recvbuf);
            if (!readPacket(client, recvbuf, recvbuflen)) {
                X_LOG0("TelemSrv", "Error reading packet");
                return;
            }

         //   X_LOG0("TelemSrv", "Bytes received: %" PRIi32, recvbuflen);

            if (client.pFile) {
            //    fwrite(recvbuf, recvbuflen, 1, client.pFile);
            }

            if (!processPacket(client, reinterpret_cast<uint8_t*>(recvbuf))) {
                X_LOG0("TelemSrv", "Failed to process packet");
                return;
            }
        }
    }


} // namespace



X_NAMESPACE_BEGIN(telemetry)

Server::Server(core::MemoryArenaBase* arena) :
    arena_(arena)
{

}

Server::~Server()
{

}

bool Server::run()
{
    if (!winSockInit()) {
        return false;
    }

    // have the server listen...
    if (!listen()) {
        return false;
    }

    winSockShutDown();
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

    tt_int32 sock_opt = 1024 * 512;
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

        // Accept a client socket
        clientSocket = platform::accept(listenSocket, NULL, NULL);
        if (clientSocket == INV_SOCKET) {
            platform::closesocket(listenSocket);
            return false;
        }

        X_LOG0("TelemSrv", "Client connected");

        Client client;
        client.socket = clientSocket;
        client.pFile = fopen("stream.dump", "wb");

        handleClient(client);


        if (client.pFile) {
            fclose(client.pFile);
        }

        platform::closesocket(clientSocket);
    }

    // clean up socket.
    platform::closesocket(listenSocket);

    return true;

}

X_NAMESPACE_END