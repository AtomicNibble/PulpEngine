#pragma once

#include <Containers\Array.h>
#include <Platform\Pipe.h>
#include <../AssetDB/AssetDB.h>


X_NAMESPACE_DECLARE(ProtoBuf,
	namespace AssetDB {
		class Request;
		class ModInfo;
		class AssetExists;
		class AddAsset;
		class DeleteAsset;
		class RenameAsset;
		class UpdateAsset;
		enum Result;
	}
);


X_NAMESPACE_BEGIN(assetServer)


class AssetServer : public core::ThreadAbstract
{
public:
	// used for reading / writing msg's
	static const size_t BUF_LENGTH = 0x200;
	typedef std::array<uint8_t, BUF_LENGTH> ResponseBuffer;

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
		bool readBuf(core::Array<uint8_t>& buf, size_t size);

		bool sendRequestFail(std::string& errMsg);
		bool sendRequestOk(ProtoBuf::AssetDB::Result res);

		bool writeAndFlushBuf(const uint8_t* pBuf, size_t len);

	private:
		core::MemoryArenaBase* arena_;
		AssetServer& as_;
		core::IPC::Pipe pipe_;
	};

public:
	AssetServer(core::MemoryArenaBase* arena);
	~AssetServer();

	void Run(bool blocking = true);

private:
	void Run_Internal(void);

	core::Thread::ReturnValue ThreadRun(const core::Thread& thread) X_FINAL;

	void ModInfo(const ProtoBuf::AssetDB::ModInfo& modInfo, ResponseBuffer& outputBuffer);
	void AssetExsists(const ProtoBuf::AssetDB::AssetExists& exists, ResponseBuffer& outputBuffer);
	bool AddAsset(const ProtoBuf::AssetDB::AddAsset& add, std::string& errOut, ProtoBuf::AssetDB::Result& res);
	bool DeleteAsset(const ProtoBuf::AssetDB::DeleteAsset& del, std::string& errOut, ProtoBuf::AssetDB::Result& res);
	bool RenameAsset(const ProtoBuf::AssetDB::RenameAsset& rename, std::string& errOut, ProtoBuf::AssetDB::Result& res);
	bool UpdateAsset(const ProtoBuf::AssetDB::UpdateAsset& update, core::Array<uint8_t>& data, std::string& errOut, ProtoBuf::AssetDB::Result& res);

private:
	template<typename MessageT>
	void writeError(MessageT& response, ResponseBuffer& outputBuffer, const char* pMsg);

private:
	core::MemoryArenaBase* arena_;
	core::CriticalSection lock_;
	assetDb::AssetDB db_;

	bool threadStarted_;
};


X_NAMESPACE_END