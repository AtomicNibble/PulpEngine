#pragma once

#include "IServer.h"
#include <Containers/FixedHashTable.h>

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

    struct PackedSourceInfo
    {
        union
        {
            struct Packed
            {
                uint16_t lineNo;
                uint16_t idxFunction;
                uint16_t idxFile;
                uint16_t idxFmt;
            } raw;

            uint64_t packed;
        };
    };

    static constexpr size_t MAX_LOCKS = 256;

    using StringIdxMap = core::FixedHashTable<core::string, int32_t>;
    using IndexArr = core::Array<uint16_t>;

public:
    TraceDB() :
        cmdInsertZone(con),
        cmdInsertString(con),
        cmdInsertStringDyn(con),
        cmdInsertTickInfo(con),
        cmdInsertLock(con),
        cmdInsertLockTry(con),
        cmdInsertLockState(con),
        cmdInsertThreadName(con),
        cmdInsertLockName(con),
        cmdInsertMeta(con),
        cmdInsertMemory(con),
        cmdInsertMessage(con),
        stringMap(g_TelemSrvLibArena, 1024 * 64),
        indexMap(g_TelemSrvLibArena, 1024 * 8)
    {
        std::fill(indexMap.begin(), indexMap.end(), std::numeric_limits<uint16_t>::max());
    }

    bool createDB(core::Path<char>& path);
    bool openDB(core::Path<char>& path);
    bool createIndexes(void);

    template<typename T>
    bool setMeta(const char* pName, T value);
    void insertLockIfMissing(uint64_t lockHandle);

    int32_t handleDataPacketTickInfo(const DataPacketTickInfo* pData);
    int32_t handleDataPacketStringTableAdd(const DataPacketStringTableAdd* pData);
    int32_t handleDataPacketZone(const DataPacketZone* pData);
    int32_t handleDataPacketLockTry(const DataPacketLockTry* pData);
    int32_t handleDataPacketLockState(const DataPacketLockState* pData);
    int32_t handleDataPacketLockSetName(const DataPacketLockSetName* pData);
    int32_t handleDataPacketThreadSetName(const DataPacketThreadSetName* pData);
    int32_t handleDataPacketLockCount(const DataPacketLockCount* pData);
    int32_t handleDataPacketMemAlloc(const DataPacketMemAlloc* pData);
    int32_t handleDataPacketMemFree(const DataPacketMemFree* pData);
    int32_t handleDataPacketMessage(const DataPacketMessage* pData);
    int32_t handleDataPacketCallStack(const DataPacketCallStack* pData);

private:
    bool setPragmas(void);
    bool createTables(void);

    uint16_t addString(core::string_view str);
    uint16_t getStringIndex(uint16_t strIdx) const;
    uint16_t getFmtStringIndex(const DataPacketBaseArgData* pPacket, int32_t packetSize, uint16_t strIdxFmt);

public:
    sql::SqlLiteDb con;

private:
    sql::SqlLiteCmd cmdInsertZone;
    sql::SqlLiteCmd cmdInsertString;
    sql::SqlLiteCmd cmdInsertStringDyn;
    sql::SqlLiteCmd cmdInsertTickInfo;
    sql::SqlLiteCmd cmdInsertLock;
    sql::SqlLiteCmd cmdInsertLockTry;
    sql::SqlLiteCmd cmdInsertLockState;
    sql::SqlLiteCmd cmdInsertThreadName;
    sql::SqlLiteCmd cmdInsertLockName;
    sql::SqlLiteCmd cmdInsertMeta;
    sql::SqlLiteCmd cmdInsertMemory;
    sql::SqlLiteCmd cmdInsertMessage;

    core::FixedArray<uint64_t, MAX_LOCKS> lockSet;

    StringIdxMap stringMap;
    IndexArr indexMap;
};

struct TraceStream
{
    TraceStream(TraceStream&& oth) :
        pTrace(std::move(oth.pTrace)),
        db(std::move(oth.db))
    {
    }
    TraceStream() :
        pTrace(nullptr)
    {
    }

    const Trace* pTrace;

    TraceDB db;
};

enum class ClientType
{
    Unknown,
    TraceStream,
    Viewer
};

struct ClientConnection
{
    using TraceStreamArr = core::Array<TraceStream>;

    ClientConnection(core::MemoryArenaBase* arena) :
        traces(arena)
    {
        core::zero_object(clientVer);
        socket = INV_SOCKET;
        type = ClientType::Unknown;

        cmpBufBegin = 0;
        cmpBufEnd = 0;

#if X_DEBUG
        core::zero_object(cmpRingBuf);
#endif // X_DEBUG
    }

    platform::SOCKET socket;
    VersionInfo clientVer;
    core::string hostName;
    ClientType type;

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
    
    bool handleQueryTraceInfo(ClientConnection& client, uint8_t* pData);
    bool handleOpenTrace(ClientConnection& client, uint8_t* pData);
    bool handleReqTraceZoneSegment(ClientConnection& client, uint8_t* pData);
    bool handleReqTraceLocks(ClientConnection& client, uint8_t* pData);
    bool handleReqTraceStrings(ClientConnection& client, uint8_t* pData);
    bool handleReqTraceThreadNames(ClientConnection& client, uint8_t* pData);
    bool handleReqTraceLockNames(ClientConnection& client, uint8_t* pData);

private:
    core::MemoryArenaBase* arena_;
    TraceAppArr apps_;
};

X_NAMESPACE_END
