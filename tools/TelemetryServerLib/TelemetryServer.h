#pragma once

#include "IServer.h"

#include <IFileSys.h>

#include <Containers/FixedHashTable.h>
#include <Containers/ByteStream.h>

#include <Memory/AllocationPolicies/LinearAllocator.h>
#include <Memory/SimpleMemoryArena.h>
#include <Memory/VirtualMem.h>

#include <optional>

X_NAMESPACE_DECLARE(core,
    struct XFileAsync;
)

X_NAMESPACE_BEGIN(telemetry)

const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);

using StringBuf = core::StackString<MAX_STRING_LEN, char>;

struct Settings
{
    core::Path<> symbolDir;
    core::Path<> symbolTmpDir;
};


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
    static bool getMetaUInt32(sql::SqlLiteDb& db, const char* pName, uint32_t& valOut);
    static bool getMetaUInt64(sql::SqlLiteDb& db, const char* pName, uint64_t& valOut);


public:
    sql::SqlLiteDb con;
};

struct TraceStream : public TraceDB
{
    TraceStream(TraceStream&& oth) :
        TraceDB(std::move(oth)),
        traceInfo(std::move(oth.traceInfo))
    {
    }
    TraceStream() 
    {
    }

    TraceInfo traceInfo;
};

struct TraceBuilder : public TraceStream
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

    TraceBuilder(core::MemoryArenaBase* arena) :
        cmdInsertZone(con),
        cmdInsertString(con),
        cmdInsertTickInfo(con),
        cmdInsertLock(con),
        cmdInsertLockTry(con),
        cmdInsertLockState(con),
        cmdInsertThreadName(con),
        cmdInsertThreadGroup(con),
        cmdInsertThreadGroupName(con),
        cmdInsertThreadGroupSort(con),
        cmdInsertLockName(con),
        cmdInsertMeta(con),
        cmdInsertMemAlloc(con),
        cmdInsertMemFree(con),
        cmdInsertMessage(con),
        cmdInsertZoneNode(con),
        cmdInsertPlot(con),
        cmdInsertPDB(con),
        cmdInsertCallstack(con),
        stringMap(arena, 1024 * 64),
        indexMap(arena, 1024 * 8)
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
    int32_t handleDataPacketThreadSetGroup(const DataPacketThreadSetGroup* pData);
    int32_t handleDataPacketThreadSetGroupName(const DataPacketThreadSetGroupName* pData);
    int32_t handleDataPacketThreadSetGroupSort(const DataPacketThreadSetGroupSort* pData);
    int32_t handleDataPacketLockCount(const DataPacketLockCount* pData);
    int32_t handleDataPacketMemAlloc(const DataPacketMemAlloc* pData);
    int32_t handleDataPacketMemFree(const DataPacketMemFree* pData);
    int32_t handleDataPacketMessage(const DataPacketMessage* pData);
    int32_t handleDataPacketPlot(const DataPacketPlot* pData);
    int32_t handleDataPacketCallStack(const DataPacketCallStack* pData);
    int32_t handleDataPacketPDBInfo(const DataPacketPDBInfo* pData);

    core::string_view getString(int32_t strIdx) const;

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
    sql::SqlLiteCmd cmdInsertThreadGroup;
    sql::SqlLiteCmd cmdInsertThreadGroupName;
    sql::SqlLiteCmd cmdInsertThreadGroupSort;
    sql::SqlLiteCmd cmdInsertLockName;
    sql::SqlLiteCmd cmdInsertMeta;
    sql::SqlLiteCmd cmdInsertMemAlloc;
    sql::SqlLiteCmd cmdInsertMemFree;
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

struct SocketBuffer
{
    using ByteArr = core::Array<char>;
    static constexpr size_t BUFF_SIZE = 1024 * 256;

    static_assert(BUFF_SIZE >= MAX_PACKET_SIZE, "Buffer size is too small");

    SocketBuffer(core::MemoryArenaBase* arena) :
        bytesTrans(0),
        buffer(arena, BUFF_SIZE)
    {
        buf.buf = buffer.data();
        buf.len = static_cast<int32_t>(buffer.size());
    }

    void setBufferLength(void) {
        buf.buf = buffer.data() + bytesTrans;
        buf.len = static_cast<int32_t>(buffer.size() - bytesTrans);
        X_ASSERT(buf.len > 0 && buf.len <= buffer.size(), "Length is invalid")(buffer.size());
    }

    uint32_t bytesTrans;
    ByteArr buffer;
    platform::WSABUF buf;
};

struct PerClientIoData
{
    PerClientIoData(core::MemoryArenaBase* arena) :
        buffer(arena)
    {
        core::zero_object(overlapped);
        op = IOOperation::Invalid;
    }

public:
    OVERLAPPED overlapped;
    IOOperation::Enum op;
    SocketBuffer buffer;
};

static_assert(X_OFFSETOF(PerClientIoData, overlapped) == 0, "Overlapped must be at start");

class Server;

struct ClientConnection
{
    using HostStr = core::StackString<NI_MAXHOST, char>;
    using ServStr = core::StackString<NI_MAXSERV, char>;

    using TraceStreamArr = core::Array<TraceStream>;

    struct PDBData
    {
        X_DECLARE_ENUM(Status)(
            Unknown,
            Pending,
            Exsists,
            Missing,
            Error
        );

        PDBData(core::MemoryArenaBase* arena) :
            status(Status::Unknown),
            modAddr(0),
            imageSize(0),
            fileSize(0),
            age(0),
            pFile(nullptr),
            offset_(0),
            tmpBuf(arena)
        {
        }

        Status::Enum status;

        uint64_t modAddr;
        tt_uint32 imageSize;
        tt_uint32 fileSize;

        core::Guid guid;
        tt_uint32 age;

        // The path the runtime gave us.
        core::Path<> path;

        // Used when streaming from client.
        tt_uint32 offset_;
        core::XFileAsync* pFile;
        std::optional<core::XOsFileAsyncOperation> op;
        core::ByteStream tmpBuf;
    };

    using PDBDataArr = core::Array<PDBData>;

public:
    ClientConnection(Server& srv, core::MemoryArenaBase* arena) :
        srv_(srv),
        io_(arena),
        tracesStreams_(arena),
        traceBuilder_(arena),
        pdbData_(arena)
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
    bool handleReqTraceThreadGroupNames(uint8_t* pData);
    bool handleReqTraceThreadGroups(uint8_t* pData);
    bool handleReqTraceLockNames(uint8_t* pData);
    bool handleReqTraceZoneTree(uint8_t* pData);
    bool handleReqTraceMessages(uint8_t* pData);

    // Tmp public
    void sendDataToClient(const void* pData, size_t len);
private:
    void sendConnectionRejected(const char* pReason);

    // PDB stuff.
    void registerPDB(const DataPacketPDBInfo* pInfo);
    void requestMissingPDB(const DataPacketCallStack* pData);
    int32_t handleDataPacketPDB(const DataPacketPDB* pData);
    int32_t handleDataPacketPDBBlock(const DataPacketPDBBlock* pData);
    int32_t handleDataPacketPDBError(const DataPacketPDBError* pData);

    void flushCompressionBuffer(void);
    int32_t getCompressionBufferSize(void) const;
    int32_t getCompressionBufferSpace(void) const;
    void addToCompressionBuffer(const void* pData, int32_t len);

    template<typename T>
    T* addToCompressionBufferT(void);

    bool isHandleValid(int32_t handle) const;

public:
    Server& srv_;

    platform::SOCKET            socket_;
    platform::SOCKADDR_STORAGE  clientAddr_;
    HostStr                     host_;
    ServStr                     serv_;
    VersionInfo                 clientVer_;
    ClientType                  type_;

    PerClientIoData io_;

    TraceStreamArr tracesStreams_;
    TraceBuilder traceBuilder_;

    // this is used for both incoming and outgoing streams.
    // depending on if it's incoming trace data or a viewer connection.
    int32_t cmpBufBegin_;
    int32_t cmpBufEnd_;
    uint8_t cmpRingBuf_[COMPRESSION_RING_BUFFER_SIZE];

    core::Compression::LZ4StreamDecode lz4DecodeStream_;
    core::Compression::LZ4Stream lz4Stream_;

    core::V2::Job* pPendingJob_;

    PDBDataArr pdbData_;
};

struct QueryTraceInfo;
class Server : public ITelemServer
{
public:
    Server(core::MemoryArenaBase* arena);
    ~Server() X_OVERRIDE;

    bool loadApps() X_FINAL;
    bool loadAppTraces(core::string_view appName, const core::Path<>& dir);

    bool listen(void) X_FINAL;

public:
    void readfromIOCPJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pJobData);
    bool sendAppList(ClientConnection& client);
    void handleQueryTraceInfo(ClientConnection& client, const QueryTraceInfo* pHdr);
    void closeClient(ClientConnection* pClientCon);

    void addTraceForApp(const TelemFixedStr& appName, const TraceInfo& trace);
    bool getTraceForGuid(const core::Guid& guid, TraceInfo& traceOut);

    const Settings& getsettings(void) const;

private:
    core::MemoryArenaBase* arena_;
    core::CriticalSection cs_;

    Settings settings_;

    TraceAppArr apps_;
};

X_NAMESPACE_END
