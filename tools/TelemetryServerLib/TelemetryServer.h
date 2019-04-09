#pragma once

#include "IServer.h"
#include <Containers/FixedHashTable.h>

#include <Memory/AllocationPolicies/LinearAllocator.h>
#include <Memory/SimpleMemoryArena.h>
#include <Memory/VirtualMem.h>

X_NAMESPACE_BEGIN(telemetry)

const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);

using StringBuf = core::StackString<MAX_STRING_LEN, char>;


struct ZoneTree
{
    struct ZoneInfo
    {
        ZoneInfo() {
            totalTicks = 0_ui64;
            count = 0;
        }

        uint64_t totalTicks;
        int32_t count;
    };

    struct Node
    {
        Node(core::string_view name) :
            name(name.begin(), name.length())
        {
            pFirstChild = nullptr;
            pNextsibling = nullptr;
        }

        Node* pFirstChild;
        Node* pNextsibling;

        core::string name;

        ZoneInfo info;
    };

    struct NodeFlat
    {
        int32_t parentIdx;
        ZoneInfo info;
        core::string name;
    };

    using NodeFlatArr = core::Array<NodeFlat>;

    static constexpr int32_t MAX_NODES = 1024;

public:
    ZoneTree();
    ~ZoneTree();

    void addZone(const StringBuf& buf, const DataPacketZone* pData);

    void getNodes(NodeFlatArr& arr) const;
    void print(void) const;

private:
    void addNodes_r(NodeFlatArr& arr, int32_t parIdx, const Node* pNode) const;
    void print_r(const core::string& prefix, const Node* pNode) const;
    void free_r(const Node* pNode);

private:
    Node root_;

    // want a pool for these bad boys.
    core::HeapArea heap_;
    core::LinearAllocator allocator_;
    core::SimpleMemoryArena<decltype(allocator_)> arena_;
};


struct TraceDB
{
    bool openDB(core::Path<char>& path);

    bool setPragmas(void);

    static bool getStats(sql::SqlLiteDb& db, TraceStats& stats);
    static bool getMetaStr(sql::SqlLiteDb& db, const char* pName, core::string& strOut);
    static bool getMetaUInt64(sql::SqlLiteDb& db, const char* pName, uint64_t& valOut);


public:
    sql::SqlLiteDb con;
};

struct TraceStream : public TraceDB
{
    TraceStream(TraceStream&& oth) :
        TraceDB(std::move(oth)),
        pTrace(std::move(oth.pTrace))
    {
    }
    TraceStream() :
        pTrace(nullptr)
    {
    }

    const Trace* pTrace;
};

struct TraceBuilder : public TraceDB
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
                uint16_t depth;
            } raw;

            uint64_t packed;
        };
    };

    static constexpr size_t MAX_LOCKS = 256;

    using StringIdxMap = core::FixedHashTable<core::string, int32_t>;

    struct IndexStr
    {
        IndexStr(uint16_t idx, core::string str) :
            idx(idx),
            str(str)
        {}
        IndexStr() :
            idx(std::numeric_limits<uint16_t>::max())
        {}

        uint16_t idx;
        core::string str;
    };

    using IndexStrPairArr = core::Array<IndexStr>;

public:
    TraceBuilder() :
        pTrace(nullptr),
        cmdInsertZone(con),
        cmdInsertString(con),
        cmdInsertTickInfo(con),
        cmdInsertLock(con),
        cmdInsertLockTry(con),
        cmdInsertLockState(con),
        cmdInsertThreadName(con),
        cmdInsertLockName(con),
        cmdInsertMeta(con),
        cmdInsertMemory(con),
        cmdInsertMessage(con),
        cmdInsertZoneNode(con),
        cmdInsertPlot(con),
        stringMap(g_TelemSrvLibArena, 1024 * 64),
        indexMap(g_TelemSrvLibArena, 1024 * 8)
    {
        // std::fill(indexMap.begin(), indexMap.end(), std::numeric_limits<uint16_t>::max());
    }

    bool createDB(core::Path<char>& path);
    bool createIndexes(void);
    void flushZoneTree(void);

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
    int32_t handleDataPacketPlot(const DataPacketPlot* pData);
    int32_t handleDataPacketCallStack(const DataPacketCallStack* pData);

private:
    bool createTables(void);

    int32_t addString(core::string_view str);
    int32_t indexForString(core::string_view str);
    uint16_t getStringIndex(uint16_t strIdx) const;
    uint16_t getStringIndex(StringBuf& buf, const DataPacketBaseArgData* pPacket, int32_t packetSize, uint16_t strIdxFmt);

    void accumulateZoneData(const StringBuf& buf, int32_t strIdx, const DataPacketZone* pData);
    void writeZoneTree(const ZoneTree& zoneTree, int32_t setID);

public:
    const Trace* pTrace;

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
    sql::SqlLiteCmd cmdInsertMessage;
    sql::SqlLiteCmd cmdInsertZoneNode;
    sql::SqlLiteCmd cmdInsertPlot;

    core::FixedArray<uint64_t, MAX_LOCKS> lockSet;

    StringIdxMap stringMap;
    IndexStrPairArr indexMap;

    ZoneTree zoneTree_;
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
    TraceBuilder traceBuilder;

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
    bool handleReqTraceZoneTree(ClientConnection& client, uint8_t* pData);

private:
    core::MemoryArenaBase* arena_;
    TraceAppArr apps_;
};

X_NAMESPACE_END
