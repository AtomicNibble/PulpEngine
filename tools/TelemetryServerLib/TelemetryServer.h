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
        trace(std::move(oth.trace))
    {
    }
    TraceStream() 
    {
    }

    TraceInfo trace;
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
    TraceBuilder(const TraceBuilder& oth) = delete;
    TraceBuilder& operator=(const TraceBuilder& oth) = delete;

    TraceBuilder() :
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
        cmdInsertPDB(con),
        cmdInsertCallstack(con),
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
    int32_t handleDataPacketPDB(const DataPacketPDB* pData);

private:
    bool createTables(void);

    int32_t addString(core::string_view str);
    int32_t indexForString(core::string_view str);
    uint16_t getStringIndex(uint16_t strIdx) const;
    uint16_t getStringIndex(StringBuf& buf, const DataPacketBaseArgData* pPacket, int32_t packetSize, uint16_t strIdxFmt);

    void accumulateZoneData(const StringBuf& buf, int32_t strIdx, const DataPacketZone* pData);
    void writeZoneTree(const ZoneTree& zoneTree, int32_t setID);

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
    sql::SqlLiteCmd cmdInsertPDB;
    sql::SqlLiteCmd cmdInsertCallstack;

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

X_DECLARE_ENUM(IOOperation)(
    Invalid,
    Send,
    Recv
);

struct PerClientIoData
{
    PerClientIoData()
    {
        core::zero_object(overlapped);

        op = IOOperation::Invalid;
        bytesTrans = 0;
        buf.buf = recvbuf;
        buf.len = sizeof(recvbuf);
    }

    OVERLAPPED overlapped;
    IOOperation::Enum op;

    uint32_t bytesTrans;

    platform::WSABUF buf;
    char recvbuf[MAX_PACKET_SIZE];
};

static_assert(X_OFFSETOF(PerClientIoData, overlapped) == 0, "Overlapped must be at start");

class Server;

struct ClientConnection
{
    using HostStr = core::StackString<NI_MAXHOST, char>;
    using ServStr = core::StackString<NI_MAXSERV, char>;

    using TraceStreamArr = core::Array<TraceStream>;

public:
    ClientConnection(Server& srv, core::MemoryArenaBase* arena) :
        srv_(srv),
        traces_(arena)
    {
        core::zero_object(clientVer_);
        socket_ = INV_SOCKET;
        type_ = ClientType::Unknown;

        cmpBufBegin_ = 0;
        cmpBufEnd_ = 0;

#if X_DEBUG
        core::zero_object(cmpRingBuf_);
#endif // X_DEBUG

        pPendingJob_ = nullptr;
    }

    void flush(void);
    void disconnect(void);

    void processNetPacketJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pJobData);
    void processDataStreamJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pJobData);

    bool handleConnectionRequest(uint8_t* pData);
    bool handleConnectionRequestViewer(uint8_t* pData);

    bool handleDataStream(uint8_t* pData);
    void processDataStream(uint8_t* pData, int32_t len);

    bool handleQueryTraceInfo(uint8_t* pData);
    bool handleOpenTrace(uint8_t* pData);

    bool handleReqTraceZoneSegment(uint8_t* pData);
    bool handleReqTraceLocks(uint8_t* pData);
    bool handleReqTraceStrings(uint8_t* pData);
    bool handleReqTraceThreadNames(uint8_t* pData);
    bool handleReqTraceLockNames(uint8_t* pData);
    bool handleReqTraceZoneTree(uint8_t* pData);

    // Tmp public
    void sendDataToClient(const void* pData, size_t len);
private:
    void sendConnectionRejected(const char* pReason);

    void flushCompressionBuffer(void);
    int32_t getCompressionBufferSize(void) const;
    int32_t getCompressionBufferSpace(void) const;
    void addToCompressionBuffer(const void* pData, int32_t len);

    template<typename T>
    T* addToCompressionBufferT(void);

public:
    Server& srv_;

    platform::SOCKET            socket_;
    platform::SOCKADDR_STORAGE  clientAddr_;
    HostStr                     host_;
    ServStr                     serv_;
    VersionInfo                 clientVer_;
    ClientType                  type_;

    PerClientIoData io_;

    TraceStreamArr traces_;
    TraceBuilder traceBuilder_;

    // this is used for both incoming and outgoing streams.
    // depending on if it'sincoming trace data or a viewer connection.
    int32_t cmpBufBegin_;
    int32_t cmpBufEnd_;
    uint8_t cmpRingBuf_[COMPRESSION_RING_BUFFER_SIZE];

    core::Compression::LZ4StreamDecode lz4DecodeStream_;
    core::Compression::LZ4Stream lz4Stream_;

    core::V2::Job* pPendingJob_;
};

struct QueryTraceInfo;
class Server : public ITelemServer
{
public:
    Server(core::MemoryArenaBase* arena);
    ~Server() X_OVERRIDE;

    bool loadApps() X_FINAL;
    bool loadAppTraces(core::Path<> appName, const core::Path<>& dir);

    bool listen(void) X_FINAL;

public:
    void addTraceForApp(const TelemFixedStr& appName, TraceInfo& trace);
    bool sendAppList(ClientConnection& client);

    void handleQueryTraceInfo(ClientConnection& client, const QueryTraceInfo* pHdr);
    bool getTraceForGuid(const core::Guid& guid, TraceInfo& traceOut);

    void readfromIOCPJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pJobData);
    void closeClient(ClientConnection* pClientCon);

private:
    core::MemoryArenaBase* arena_;
    core::CriticalSection cs_;

    TraceAppArr apps_;
};

X_NAMESPACE_END
