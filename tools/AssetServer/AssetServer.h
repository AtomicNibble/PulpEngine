#pragma once

#include <Containers\Array.h>
#include <Platform\Pipe.h>
#include <Util\UniquePointer.h>

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_DECLARE(ProtoBuf,
    namespace AssetDB {
        class Request;
        class ConverterInfoReqest;
        class ModInfo;
        class AssetExists;
        class AddAsset;
        class DeleteAsset;
        class RenameAsset;
        class UpdateAsset;
    });

X_NAMESPACE_BEGIN(assetServer)

class AssetServer : public core::ThreadAbstract
{
public:
    // used for reading / writing msg's
    static const size_t BUF_LENGTH = assetDb::api::MESSAGE_BUFFER_SIZE;
    typedef core::FixedArray<uint8_t, BUF_LENGTH> ResponseBuffer;
    typedef core::Array<uint8_t> DataArr;
    typedef assetDb::AssetDB::AssetType AssetType;

private:
    class Client
    {
    public:
        Client(AssetServer& as, core::MemoryArenaBase* arena);
        ~Client() = default;

    public:
        bool connect(void);
        bool listen(void);

    private:
        bool readRequest(ProtoBuf::AssetDB::Request& request);
        bool readBuf(DataArr& buf, size_t size);

        void writeError(ResponseBuffer& outputBuffer, const char* pErrMsg, ...);

        bool writeAndFlushBuf(ResponseBuffer& outputBuffer);

    private:
        core::MemoryArenaBase* arena_;
        AssetServer& as_;
        core::IPC::Pipe pipe_;
    };

    class ClientThread : private core::ThreadAbstract
    {
    public:
        typedef core::UniquePointer<Client> ClientPtr;

    public:
        ClientThread(ClientPtr client, core::MemoryArenaBase* arena);

        bool listen(void);

    private:
        core::Thread::ReturnValue ThreadRun(const core::Thread& thread) X_FINAL;

    private:
        ClientPtr client_;
        core::MemoryArenaBase* arena_;
    };

public:
    AssetServer(core::MemoryArenaBase* arena);
    ~AssetServer();

    void Run(bool blocking = true);

private:
    bool Run_Internal(void);

    core::Thread::ReturnValue ThreadRun(const core::Thread& thread) X_FINAL;

    void ConverterInfo(const ProtoBuf::AssetDB::ConverterInfoReqest& modInfo, ResponseBuffer& outputBuffer);
    void ModInfo(const ProtoBuf::AssetDB::ModInfo& modInfo, ResponseBuffer& outputBuffer);
    void AssetExsists(const ProtoBuf::AssetDB::AssetExists& exists, ResponseBuffer& outputBuffer);
    void AddAsset(const ProtoBuf::AssetDB::AddAsset& add, ResponseBuffer& outputBuffer);
    void DeleteAsset(const ProtoBuf::AssetDB::DeleteAsset& del, ResponseBuffer& outputBuffer);
    void RenameAsset(const ProtoBuf::AssetDB::RenameAsset& rename, ResponseBuffer& outputBuffer);
    void UpdateAsset(const ProtoBuf::AssetDB::UpdateAsset& update, core::Array<uint8_t>& data, ResponseBuffer& outputBuffer);

private:
    template<typename MessageT>
    void writeError(MessageT& response, ResponseBuffer& outputBuffer, const char* pMsg);

    template<typename MessageT>
    void writeResponse(MessageT& response, ResponseBuffer& outputBuffer, assetDb::AssetDB::Result::Enum res);

private:
    core::MemoryArenaBase* arena_;
    core::CriticalSection lock_;
    assetDb::AssetDB db_;

    bool threadStarted_;
};

X_NAMESPACE_END