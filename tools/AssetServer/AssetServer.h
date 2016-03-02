#pragma once

#include <Platform\Pipe.h>
#include <../AssetDB/AssetDB.h>


X_NAMESPACE_DECLARE(ProtoBuf,
	namespace AssetDB {
		class Request;
		class AddAsset;
		class DeleteAsset;
		class RenameAsset;
	}
);


X_NAMESPACE_BEGIN(assetServer)


class AssetServer
{
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

		bool sendRequestFail(std::string& errMsg);
		bool sendRequestOk(void);

		bool writeAndFlushBuf(const uint8_t* pBuf, size_t len);

	private:
		AssetServer& as_;
		core::IPC::Pipe pipe_;
	};

public:
	AssetServer();
	~AssetServer();

	void Run(void);

private:
	bool AddAsset(const ProtoBuf::AssetDB::AddAsset& add, std::string& errOut);
	bool DeleteAsset(const ProtoBuf::AssetDB::DeleteAsset& del, std::string& errOut);
	bool RenameAsset(const ProtoBuf::AssetDB::RenameAsset& rename, std::string& errOut);

private:
	core::CriticalSection lock_;

	assetDb::AssetDB db_;
};


X_NAMESPACE_END