#include "stdafx.h"
#include "AssetServer.h"

#include <Containers\Array.h>

X_DISABLE_WARNING(4244)
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
X_ENABLE_WARNING(4244)


#include "..\protobuf\src\assetdb.pb.h"

#if X_DEBUG
X_LINK_LIB("libprotobufd")
#else
X_LINK_LIB("libprotobuf")
#endif // !X_DEBUG


// need assetDB.
X_LINK_LIB("engine_AssetDb")


X_NAMESPACE_BEGIN(assetServer)

namespace
{

	bool ReadDelimitedFrom(google::protobuf::io::ZeroCopyInputStream* rawInput,
		google::protobuf::MessageLite* message, bool* cleanEof)
	{
		google::protobuf::io::CodedInputStream input(rawInput);
		const int start = input.CurrentPosition();
		if (cleanEof) {
			*cleanEof = false;
		}

		// Read the size.
		uint32_t size;
		if (!input.ReadVarint32(&size))
		{
			if (cleanEof) {
				*cleanEof = input.CurrentPosition() == start;
			}
			return false;
		}
		// Tell the stream not to read beyond that size.
		google::protobuf::io::CodedInputStream::Limit limit = input.PushLimit(size);

		// Parse the message.
		if (!message->MergeFromCodedStream(&input)) {
			return false;
		}
		if (!input.ConsumedEntireMessage()) {
			return false;
		}

		// Release the limit.
		input.PopLimit(limit);
		return true;
	}


	bool WriteDelimitedTo(const google::protobuf::MessageLite& message,
		google::protobuf::io::ZeroCopyOutputStream* rawOutput)
	{
		// We create a new coded stream for each message. 
		google::protobuf::io::CodedOutputStream output(rawOutput);

		// Write the size.
		const int size = message.ByteSize();
		output.WriteVarint32(size);

		uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
		if (buffer != nullptr)
		{
			// Optimization:  The message fits in one buffer, so use the faster
			// direct-to-array serialization path.
			message.SerializeWithCachedSizesToArray(buffer);
		}

		else
		{
			// Slightly-slower path when the message is multiple buffers.
			message.SerializeWithCachedSizes(&output);
			if (output.HadError()) {
				X_ERROR("Proto", "Failed to write msg to output stream");
				return false;
			}
		}

		return true;
	}


	bool MapAssetType(ProtoBuf::AssetDB::AssetType type, assetDb::AssetDB::AssetType::Enum& typeOut)
	{
		switch (type)
		{
		case ProtoBuf::AssetDB::AssetType::ANIM:
			typeOut = assetDb::AssetDB::AssetType::ANIM;
			break;
		case ProtoBuf::AssetDB::AssetType::MODEL:
			typeOut = assetDb::AssetDB::AssetType::MODEL;
			break;
		default:
			X_ASSERT_UNREACHABLE();
			return false;
			break;
		}

		return true;
	}

} // namespace




AssetServer::Client::Client(AssetServer& as) :
	as_(as)
{
}


bool AssetServer::Client::connect(void)
{
	// create pipe for a new client to connect to.
	if (!pipe_.create("\\\\.\\pipe\\" X_ENGINE_NAME "_AssetServer",
		core::IPC::Pipe::CreateMode::DUPLEX,
		core::IPC::Pipe::PipeMode::MESSAGE_RW,
		10,
		512,
		512,
		core::TimeVal::fromMS(100)
		)) {
		X_ERROR("AssetServer", "Failed to create pipe");
		return false;
	}

	// wait for a client to connect.
	if (!pipe_.connect()) {
		X_WARNING("AssetServer", "Client failed to connect");
		return false;
	}

	return true;
}

bool AssetServer::Client::listen(void)
{
	ProtoBuf::AssetDB::Request request;

	// loop until error.
	while (1)
	{
		request.Clear();

		// wait for a request.
		if (!readRequest(request)) {
			break;
		}

		bool ok = false;
		std::string err;

		// have the asset server do the work of hte request.
		// it may also set hte error string to return extra info.
		ProtoBuf::AssetDB::Request::MsgCase type = request.msg_case();
		switch (type)
		{
		case ProtoBuf::AssetDB::Request::kAdd:
			ok = as_.AddAsset(request.add(), err);
			break;
		case ProtoBuf::AssetDB::Request::kDel:
			ok = as_.DeleteAsset(request.del(), err);
			break;
		case ProtoBuf::AssetDB::Request::kRename:
			ok = as_.RenameAsset(request.rename(), err);
			break;
		case ProtoBuf::AssetDB::Request::kUpdate:
		{
			// not sure where is best place to put this is.
			// here or inside UpdateAsset() :Z
			core::Array<uint8_t> buf(g_arena);
			if (request.update().has_datasize()) 
			{
				uint32_t dataSize = request.update().datasize();

				if (!readBuf(buf, dataSize)) {
					err = "Failed to read buffer after UpdateAssert Msg.";
					goto fail;
				}
			}
			ok = as_.UpdateAsset(request.update(), buf, err);
		fail:;
		}
			break;
		default:
			err = "Unkown request msg type";
			X_ERROR("AssetServer", "Unknown request msg type");
			break;
		}

		// send response.
		if (ok) {
			if(!sendRequestOk()) {
				break;
			}
		}
		else {
			if (!sendRequestFail(err)) {
				break;
			}
		}
		
	}

	// disconnect.
	pipe_.disconnect();

	X_WARNING("AssetServer", "Dropped client connection..");
	return true;
}

bool AssetServer::Client::readRequest(ProtoBuf::AssetDB::Request& request)
{
	const size_t bufLength = 0x200;
	uint8_t buffer[bufLength];

	size_t bytesRead;
	bool cleanEof;

	if (!pipe_.read(buffer, sizeof(buffer), &bytesRead)) {
		X_ERROR("AssetServer", "Failed to read msg");
		return false;
	}

	google::protobuf::io::ArrayInputStream arrayInput(buffer, 
		safe_static_cast<int32_t, size_t>(bytesRead));

	if (!ReadDelimitedFrom(&arrayInput, &request, &cleanEof)) {
		X_ERROR("AssetServer", "Failed to parse msg");
		return false;
	}

	return true;
}

bool AssetServer::Client::readBuf(core::Array<uint8_t>& buf, size_t size)
{
	size_t bytesRead, bytesReadTotal;

	if (size == 0) {
		return true;
	}

	buf.resize(size);
	bytesReadTotal = 0;

	while (true)
	{
		const size_t bufSpace = size - bytesReadTotal;

		bytesRead = 0;
		if (!pipe_.read(&buf[bytesReadTotal], bufSpace, &bytesRead)) {
			X_ERROR("AssetServer", "Failed to read msg-buf");
			buf.clear();
			return false;
		}

		bytesReadTotal += bytesRead;
		if (bytesReadTotal >= size) {
			break;
		}
	}

	return true;
}

bool AssetServer::Client::sendRequestFail(std::string& errMsg)
{
	const size_t bufLength = 0x200;
	uint8_t buffer[bufLength];

	ProtoBuf::AssetDB::Reponse response;
	response.set_error(errMsg);
	response.set_result(ProtoBuf::AssetDB::Reponse_Result_FAIL);

	google::protobuf::io::ArrayOutputStream arrayOutput(buffer, bufLength);
	if (!WriteDelimitedTo(response, &arrayOutput)) {
		X_ERROR("AssetServer", "Failed to create response msg");
		return false;
	}

	return writeAndFlushBuf(buffer, safe_static_cast<size_t, int64_t>(arrayOutput.ByteCount()));
}

bool AssetServer::Client::sendRequestOk(void)
{
	const size_t bufLength = 0x200;
	uint8_t buffer[bufLength];

	ProtoBuf::AssetDB::Reponse response;
	response.set_error("");
	response.set_result(ProtoBuf::AssetDB::Reponse_Result_OK);

	google::protobuf::io::ArrayOutputStream arrayOutput(buffer, bufLength);
	if (!WriteDelimitedTo(response, &arrayOutput)) {
		X_ERROR("AssetServer", "Failed to create response msg");
		return false;
	}

	return writeAndFlushBuf(buffer, safe_static_cast<size_t, int64_t>(arrayOutput.ByteCount()));
}

bool AssetServer::Client::writeAndFlushBuf(const uint8_t* pBuf, size_t len)
{
	if (!pipe_.write(pBuf, len)) {
		X_ERROR("AssetServer", "Failed to write response msg");
		return false;
	}

	if (!pipe_.flush()) {
		X_ERROR("AssetServer", "Failed to flush response msg");
		return false;
	}

	return true;
}

// -------------------------------------------


AssetServer::AssetServer() :
	lock_(50),
	threadStarted_(false)
{

}

AssetServer::~AssetServer()
{
	if (threadStarted_) {
		CancelSynchronousIo();
		Stop();
		Join();
	}

	// shut down the slut.
	google::protobuf::ShutdownProtobufLibrary();
}


void AssetServer::Run(bool blocking)
{
	if (blocking) {
		Run_Internal();
	}
	else {	
		Create("AssetServerWorker");
		Start();

		threadStarted_ = true;
	}
}

void AssetServer::Run_Internal(void)
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	db_.OpenDB();
	db_.CreateTables();

	while (1)
	{
		Client client(*this);

		client.connect();
		client.listen();
	}

	db_.CloseDB();
}

core::Thread::ReturnValue AssetServer::ThreadRun(const core::Thread& thread)
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	db_.OpenDB();
	db_.CreateTables();

	while (thread.ShouldRun())
	{
		Client client(*this);

		client.connect();
		client.listen();
	}

	db_.CloseDB();

	return core::Thread::ReturnValue(0);
}


bool AssetServer::AddAsset(const ProtoBuf::AssetDB::AddAsset& add, std::string& errOut)
{
	// map type.
	assetDb::AssetDB::AssetType::Enum type;

	if (!MapAssetType(add.type(), type)) {
		errOut = "Unknown asset type in AddAsset()";
		X_ERROR("Assetserver", errOut.c_str());
		return false;
	}
	
	core::string name(add.name().data(), add.name().length());

	X_LOG1("AssetServer", "AddAsset: %s name: \"%s\"", assetDb::AssetDB::AssetType::ToString(type), name.c_str());

	core::CriticalSection::ScopedLock slock(lock_);

	assetDb::AssetDB::Result::Enum res = db_.AddAsset(type, name);
	if (res != assetDb::AssetDB::Result::OK) {
		errOut = "Failed to add asset. Err: ";
		errOut += assetDb::AssetDB::Result::ToString(res);
		return false;
	}

	return true;
}

bool AssetServer::DeleteAsset(const ProtoBuf::AssetDB::DeleteAsset& del, std::string& errOut)
{
	// map type.
	assetDb::AssetDB::AssetType::Enum type;

	if (!MapAssetType(del.type(), type)) {
		errOut = "Unknown asset type in DeleteAsset()";
		X_ERROR("Assetserver", errOut.c_str());
		return false;
	}

	core::string name(del.name().data(), del.name().length());

	X_LOG1("AssetServer", "DeleteAsset: %s name: \"%s\"", assetDb::AssetDB::AssetType::ToString(type), name.c_str());

	core::CriticalSection::ScopedLock slock(lock_);

	assetDb::AssetDB::Result::Enum res = db_.DeleteAsset(type, name);
	if (res != assetDb::AssetDB::Result::OK) {
		errOut = "Failed to delete asset. Err: ";
		errOut += assetDb::AssetDB::Result::ToString(res);
		return false;
	}

	return true;
}

bool AssetServer::RenameAsset(const ProtoBuf::AssetDB::RenameAsset& rename, std::string& errOut)
{
	// map type.
	assetDb::AssetDB::AssetType::Enum type;

	if (!MapAssetType(rename.type(), type)) {
		errOut = "Unknown asset type in RenameAsset()";
		X_ERROR("Assetserver", errOut.c_str());
		return false;
	}

	core::string name(rename.name().data(), rename.name().length());
	core::string newName(rename.newname().data(), rename.newname().length());

	X_LOG1("AssetServer", "RenameAsset: %s name: \"%s\" newName: \"%s\"",
		assetDb::AssetDB::AssetType::ToString(type), name.c_str(), newName.c_str());

	core::CriticalSection::ScopedLock slock(lock_);

	assetDb::AssetDB::Result::Enum res = db_.RenameAsset(type, name, newName);
	if (res != assetDb::AssetDB::Result::OK) {
		errOut = "Failed to rename asset. Err: ";
		errOut += assetDb::AssetDB::Result::ToString(res);
		return false;
	}
	
	return true;
}

bool AssetServer::UpdateAsset(const ProtoBuf::AssetDB::UpdateAsset& update, core::Array<uint8_t>& data, std::string& errOut)
{
	// map type.
	assetDb::AssetDB::AssetType::Enum type;

	if (!MapAssetType(update.type(), type)) {
		errOut = "Unknown asset type in RenameAsset()";
		X_ERROR("Assetserver", errOut.c_str());
		return false;
	}

	core::string name(update.name().data(), update.name().length());
	core::string path, args;
	if (update.has_path()) {
		path = core::string(update.path().data(), update.path().length());
	}
	if (update.has_args()) {
		args = core::string(update.args().data(), update.args().length());
	}


	X_LOG1("AssetServer", "UpdateAsset: %s name: \"%s\"",
		assetDb::AssetDB::AssetType::ToString(type), name.c_str());

	core::CriticalSection::ScopedLock slock(lock_);

	assetDb::AssetDB::Result::Enum res = db_.UpdateAsset(type, name, data, path, args);
	if (res != assetDb::AssetDB::Result::OK) {
		errOut = "Failed to update asset. Err: ";
		errOut += assetDb::AssetDB::Result::ToString(res);
		return false;
	}

	return true;
}

X_NAMESPACE_END
