#pragma once

#include "IServer.h"

X_NAMESPACE_BEGIN(telemetry)

const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);


struct TraceDB
{
    // maybe drop this and just use -1?
    enum class MemOp
    {
        Alloc,
        Free
    };

    struct PacketSourceInfo
    {
        union
        {
            struct Packed
            {
                uint16_t lineNo;
                uint16_t idxFunction;
                uint16_t idxFile;
                uint16_t __blank;
            } raw;

            uint64_t packed;
        };
    };

    static constexpr size_t MAX_LOCKS = 256;

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

public:
    bool createDB(core::Path<char>& path);
    bool openDB(core::Path<char>& path);
    bool createIndexes(void);

    template<typename T>
    bool setMeta(const char* pName, T value);
    void insertLockIfMissing(uint64_t lockHandle);

    void handleDataPacketTickInfo(const DataPacketTickInfo* pData);
    void handleDataPacketStringTableAdd(const DataPacketStringTableAdd* pData);
    void handleDataPacketZone(const DataPacketZone* pData);
    void handleDataPacketLockTry(const DataPacketLockTry* pData);
    void handleDataPacketLockState(const DataPacketLockState* pData);
    void handleDataPacketLockSetName(const DataPacketLockSetName* pData);
    void handleDataPacketThreadSetName(const DataPacketThreadSetName* pData);
    void handleDataPacketLockCount(const DataPacketLockCount* pData);
    void handleDataPacketMemAlloc(const DataPacketMemAlloc* pData);
    void handleDataPacketMemFree(const DataPacketMemFree* pData);
    void handleDataPacketCallStack(const DataPacketCallStack* pData);

    bool getTicks(core::Array<DataPacketTickInfo>& ticks, int32_t startIdx, int32_t num);
    bool getZones(core::Array<DataPacketZone>& zones, uint64_t tickBegin, uint64_t tickEnd);

private:
    bool setPragmas(void);
    bool createTables(void);

public:
    sql::SqlLiteDb con;

private:
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

    core::FixedArray<uint64_t, MAX_LOCKS> lockSet;
};

struct TraceStream
{
    TraceStream(TraceStream&& oth) :
        pTrace(std::move(oth.pTrace)),
        db(std::move(oth.db))
    {
    }
    TraceStream() {
    }

    const Trace* pTrace;

    TraceDB db;
};

struct ClientConnection
{
    using TraceStreamArr = core::Array<TraceStream>;

    ClientConnection(core::MemoryArenaBase* arena) :
        traces(arena)
    {
        core::zero_object(clientVer);
        socket = INV_SOCKET;

        cmpBufBegin = 0;
        cmpBufEnd = 0;

#if X_DEBUG
        core::zero_object(cmpRingBuf);
#endif // X_DEBUG
    }

    platform::SOCKET socket;
    VersionInfo clientVer;
    core::string hostName;

    TraceStreamArr traces;
    TraceStream traceStrm;

    // this is used for both incoming and outgoing streams.
    // depending on if it'sincoming trace data or a viewer connection.
    int32_t cmpBufBegin;
    int32_t cmpBufEnd;
    int8_t cmpRingBuf[COMPRESSION_RING_BUFFER_SIZE];

    core::Compression::LZ4StreamDecode lz4DecodeStream;
    core::Compression::LZ4Stream lz4Stream;
};


class Server : public ITelemServer
{
public:


public:
    Server(core::MemoryArenaBase* arena);
    ~Server() X_OVERRIDE;

    bool loadApps() X_FINAL;
    bool loadAppTraces(core::Path<> appName, const core::Path<>& dir);

    bool listen(void) X_FINAL;

private:
    bool processPacket(ClientConnection& client, uint8_t* pData);
    void handleClient(ClientConnection& client);

    bool sendAppList(ClientConnection& client);

    bool handleConnectionRequest(ClientConnection& client, uint8_t* pData);
    bool handleConnectionRequestViewer(ClientConnection& client, uint8_t* pData);
    bool handleOpenTrace(ClientConnection& client, uint8_t* pData);

    bool handleReqTraceTicks(ClientConnection& client, uint8_t* pData);
    bool handleReqTraceZones(ClientConnection& client, uint8_t* pData);
    bool handleReqTraceStrings(ClientConnection& client, uint8_t* pData);

private:
    core::MemoryArenaBase* arena_;
    TraceAppArr apps_;
};

X_NAMESPACE_END