#pragma once

#include <maya\MPxCommand.h>

#include <Containers\Array.h>

#include <Platform\Pipe.h>

#include <IAssetDb.h>

X_NAMESPACE_DECLARE(ProtoBuf,
    namespace AssetDB {
        class Request;
        class Reponse;
        class AssetInfoResponse;
    });

namespace google
{
    namespace protobuf
    {
        class MessageLite;
    }
} // namespace google

X_NAMESPACE_BEGIN(maya)

class AssetDB
{
public:
    static const size_t BUF_LENGTH = assetDb::api::MESSAGE_BUFFER_SIZE;

    typedef assetDb::AssetType AssetType;
    typedef core::Array<uint8_t> DataArr;

    struct Mod
    {
        int32_t modId;
        core::string name;
        core::Path<char> outDir;
    };

    struct ConverterInfo
    {
        core::Path<char> workingDir;
    };

public:
    AssetDB();
    ~AssetDB();

    static void Init(void);
    static void ShutDown(void);
    static AssetDB* Get(void);

    bool Connect(void);

    bool GetConverterInfo(ConverterInfo& infoOut);
    bool GetModInfo(int32_t id, Mod& modOut);
    MStatus AssetExsists(AssetType::Enum type, const MString& name, int32_t* pIdOut, int32_t* pModIdOut);

    MStatus AddAsset(AssetType::Enum type, const MString& name);
    MStatus RemoveAsset(AssetType::Enum type, const MString& name);
    MStatus RenameAsset(AssetType::Enum type, const MString& name, const MString& oldName);
    MStatus UpdateAsset(AssetType::Enum type, const MString& name, const DataArr& data, bool* pUnchanged = nullptr);
    MStatus UpdateAsset(AssetType::Enum type, const MString& name, const MString& args,
        const DataArr& data, bool* pUnchanged = nullptr);

private:
    bool sendRequest(ProtoBuf::AssetDB::Request& request);
    bool sendBuf(const DataArr& data);
    bool getResponse(ProtoBuf::AssetDB::Reponse& response);
    bool readMessage(google::protobuf::MessageLite& message);

    bool ensureConnected(void);

private:
    core::IPC::Pipe pipe_;
};

class AssetDBCmd : public MPxCommand
{
    X_DECLARE_ENUM(Action)
    (ADD, REMOVE, RENAME);

    typedef AssetDB::AssetType AssetType;

public:
    AssetDBCmd();
    ~AssetDBCmd();

    virtual MStatus doIt(const MArgList& args) X_OVERRIDE;

    static void* creator(void);
    static MSyntax newSyntax(void);

private:
};

X_NAMESPACE_END