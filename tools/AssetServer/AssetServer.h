#pragma once

#include <Containers\Array.h>
#include <Platform\Pipe.h>
#include <../AssetDB/AssetDB.h>


X_NAMESPACE_DECLARE(ProtoBuf,
	namespace AssetDB {
		class Request;
		class AddAsset;
		class DeleteAsset;
		class RenameAsset;
		class UpdateAsset;
		enum Reponse_Result;
	}
);

X_NAMESPACE_BEGIN(assetServer)


class AssetServer : public core::ThreadAbstract
{
	// used for reading / writing msg's
	static const size_t BUF_LENGTH = 0x200;

	class Client
	{
	public:
		Client(AssetServer& as);
		~Client() = default;

	public:
		bool connect(void);
		bool listen(void);

	private:
		bool readRequest(ProtoBuf::AssetDB::Request& request);
		bool readBuf(core::Array<uint8_t>& buf, size_t size);

		bool sendRequestFail(std::string& errMsg);
		bool sendRequestOk(ProtoBuf::AssetDB::Reponse_Result res);

		bool writeAndFlushBuf(const uint8_t* pBuf, size_t len);

	private:
		AssetServer& as_;
		core::IPC::Pipe pipe_;
	};

public:
	AssetServer();
	~AssetServer();

	void Run(bool blocking = true);

private:
	void Run_Internal(void);

	core::Thread::ReturnValue ThreadRun(const core::Thread& thread) X_FINAL;

	bool AddAsset(const ProtoBuf::AssetDB::AddAsset& add, std::string& errOut, ProtoBuf::AssetDB::Reponse_Result& res);
	bool DeleteAsset(const ProtoBuf::AssetDB::DeleteAsset& del, std::string& errOut, ProtoBuf::AssetDB::Reponse_Result& res);
	bool RenameAsset(const ProtoBuf::AssetDB::RenameAsset& rename, std::string& errOut, ProtoBuf::AssetDB::Reponse_Result& res);
	bool UpdateAsset(const ProtoBuf::AssetDB::UpdateAsset& update, core::Array<uint8_t>& data, std::string& errOut, ProtoBuf::AssetDB::Reponse_Result& res);

private:
	core::CriticalSection lock_;
	assetDb::AssetDB db_;

	bool threadStarted_;
};


X_NAMESPACE_END