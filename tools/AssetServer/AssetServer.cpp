#include "stdafx.h"
#include "AssetServer.h"

#include <Containers\Array.h>
#include <IFileSys.h>


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

	inline core::string fromStdString(const std::string& str)
	{
		return core::string(str.data(), str.length());
	}


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

	template<size_t N>
	bool WriteDelimitedTo(const google::protobuf::MessageLite& message,
		core::FixedArray<uint8_t,N>& buf)
	{
		// just reset for now.
		buf.resize(buf.capacity());
		google::protobuf::io::ArrayOutputStream stream(buf.data(), buf.size());

		bool res = WriteDelimitedTo(message, &stream);

		buf.resize(safe_static_cast<size_t>(stream.ByteCount()));

		return res;
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
		case ProtoBuf::AssetDB::AssetType::MATERIAL:
			typeOut = assetDb::AssetDB::AssetType::MATERIAL;
			break;
		case ProtoBuf::AssetDB::AssetType::IMG:
			typeOut = assetDb::AssetDB::AssetType::IMG;
			break;
		default:
			X_ASSERT_UNREACHABLE();
			return false;
			break;
		}

		return true;
	}

	ProtoBuf::AssetDB::Result MapReturnType(assetDb::AssetDB::Result::Enum res)
	{
		switch (res)
		{
		case assetDb::AssetDB::Result::OK:
			return ProtoBuf::AssetDB::Result::OK;
		case assetDb::AssetDB::Result::NAME_TAKEN:
			return ProtoBuf::AssetDB::Result::NAME_TAKEN;
		case assetDb::AssetDB::Result::NOT_FOUND:
			return ProtoBuf::AssetDB::Result::NOT_FOUND;
		case assetDb::AssetDB::Result::UNCHANGED:
			return ProtoBuf::AssetDB::Result::UNCHANGED;
		case assetDb::AssetDB::Result::ERROR:
			return ProtoBuf::AssetDB::Result::ERROR;

		default:
			X_ASSERT_UNREACHABLE();
			break;
		}

		return ProtoBuf::AssetDB::Result::ERROR;
	}


} // namespace




AssetServer::Client::Client(AssetServer& as, core::MemoryArenaBase* arena) :
	arena_(arena),
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
		assetDb::api::MESSAGE_BUFFER_SIZE,
		assetDb::api::MESSAGE_BUFFER_SIZE,
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
	ResponseBuffer responseBuf;

	// loop until error.
	while (1)
	{
		request.Clear();
		responseBuf.clear();

		// wait for a request.
		if (!readRequest(request)) {
			break;
		}

		// have the asset server do the work of hte request.
		// it may also set hte error string to return extra info.
		ProtoBuf::AssetDB::Request::MsgCase type = request.msg_case();
		switch (type)
		{
		case ProtoBuf::AssetDB::Request::kAdd:
			as_.AddAsset(request.add(), responseBuf);
			break;
		case ProtoBuf::AssetDB::Request::kDel:
			as_.DeleteAsset(request.del(), responseBuf);
			break;
		case ProtoBuf::AssetDB::Request::kRename:
			as_.RenameAsset(request.rename(), responseBuf);
			break;
		case ProtoBuf::AssetDB::Request::kExists:
			as_.AssetExsists(request.exists(), responseBuf);
			break;
		case ProtoBuf::AssetDB::Request::kModInfo:
			as_.ModInfo(request.modinfo(), responseBuf);
			break;
		case ProtoBuf::AssetDB::Request::kConInfo:
			as_.ConverterInfo(request.coninfo(), responseBuf);
			break;
		case ProtoBuf::AssetDB::Request::kUpdate:
		{
			// not sure where is best place to put this is.
			// here or inside UpdateAsset() :Z
			DataArr buf(arena_);
			if (request.update().has_datasize()) 
			{
				uint32_t dataSize = request.update().datasize();

				if (!readBuf(buf, dataSize)) {
					writeError(responseBuf, "Failed to read buffer after UpdateAssert Msg.");
					break;
				}
			}

			as_.UpdateAsset(request.update(), buf, responseBuf);
		}
			break;
		default:
			writeError(responseBuf, "Unknown request msg type '%i'", type);
			break;
		}

		if (!writeAndFlushBuf(responseBuf))
		{
			break;
		}
	}

	// disconnect.
	pipe_.disconnect();

	X_WARNING("AssetServer", "Dropped client connection..");
	return true;
}

bool AssetServer::Client::readRequest(ProtoBuf::AssetDB::Request& request)
{
	uint8_t buffer[BUF_LENGTH];
	size_t bytesRead;

	if (!pipe_.read(buffer, sizeof(buffer), &bytesRead)) {
		X_ERROR("AssetServer", "Failed to read msg");
		return false;
	}

	// we on;y read buffer size, so if we get a message that fills buffer
	// I don't think we handle reciving it in sections so error.
	if (bytesRead >= BUF_LENGTH) {
		X_ERROR("AssetServer", "Msg too big, increase bugger size: %" PRIuS " recived: %" PRIuS, BUF_LENGTH, bytesRead);
		return false;
	}

	google::protobuf::io::ArrayInputStream arrayInput(buffer, safe_static_cast<int32_t>(bytesRead));
	bool cleanEof;

	if (!ReadDelimitedFrom(&arrayInput, &request, &cleanEof)) {
		X_ERROR("AssetServer", "Failed to parse msg");
		return false;
	}

	return true;
}

bool AssetServer::Client::readBuf(DataArr& buf, size_t size)
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

void AssetServer::Client::writeError(ResponseBuffer& outputBuffer, const char* pErrMsg, ...)
{
	core::StackString<2048> msg;

	va_list args;
	va_start(args, pErrMsg);
	msg.appendFmt(pErrMsg, args);
	va_end(args);

	X_ERROR("AssetServer", "Error: %s", msg.c_str());

	ProtoBuf::AssetDB::Reponse response;
	response.set_error(msg.c_str());	
	response.set_result(ProtoBuf::AssetDB::Result::ERROR);

	if (!WriteDelimitedTo(response, outputBuffer)) {
		X_ERROR("AssetServer", "Failed to create response msg");
		return;
	}
}

bool AssetServer::Client::writeAndFlushBuf(ResponseBuffer& outputBuffer)
{
	if (!pipe_.write(outputBuffer.data(), outputBuffer.size())) {
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


AssetServer::ClientThread::ClientThread(ClientPtr client, core::MemoryArenaBase* arena) :
	client_(std::move(client)),
	arena_(arena)
{

}


bool AssetServer::ClientThread::listen(void)
{
	Create("ClientThread");
	Start();
	return true;
}


core::Thread::ReturnValue AssetServer::ClientThread::ThreadRun(const core::Thread& thread)
{
	client_->listen();

	X_LOG1("AssetServer", "Client disconnected");
	X_DELETE(this, arena_);
	return core::Thread::ReturnValue(0);
}


// -------------------------------------------


AssetServer::AssetServer(core::MemoryArenaBase* arena) :
	arena_(arena),
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

bool AssetServer::Run_Internal(void)
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	if (!db_.OpenDB()) {
		return false;
	}

	// spawn off new clients for each connection.
	// ideally handle all clients on single thread by performnce requirement for this is pretty much none exsistent.
	// maybe 4-5 clients max.
	while (1)
	{
		auto client = core::makeUnique<Client>(arena_, *this, arena_);

		if (client->connect())
		{
			X_LOG1("AssetServer", "Client connected");

			auto* pClientThread = X_NEW(ClientThread, arena_, "ClientThread")(std::move(client), arena_);

			pClientThread->listen();
		}
	}

	db_.CloseDB();
	return true;
}

core::Thread::ReturnValue AssetServer::ThreadRun(const core::Thread& thread)
{
	if (!Run_Internal()) {
		return core::Thread::ReturnValue(-1);
	}

	return core::Thread::ReturnValue(0);
}

void AssetServer::ConverterInfo(const ProtoBuf::AssetDB::ConverterInfoReqest& modInfo, ResponseBuffer& outputBuffer)
{
	ProtoBuf::AssetDB::ConverterInfoResponse response;

	// TODO: utf-8 encode this?
	auto workingDir = gEnv->pFileSys->getWorkingDirectory();
	core::Path<char> narrowPath(workingDir);

	response.set_result(ProtoBuf::AssetDB::Result::OK);
	response.set_error("");
	response.set_workingdir(narrowPath.c_str());

	if (!WriteDelimitedTo(response, outputBuffer)) {
		writeError(response, outputBuffer, "Failed to create response msg");
		return;
	}
}

void AssetServer::ModInfo(const ProtoBuf::AssetDB::ModInfo& modInfo, ResponseBuffer& outputBuffer)
{
	ProtoBuf::AssetDB::ModInfoResponse response;

	if (!modInfo.has_modid()) {
		writeError(response, outputBuffer, "missing modId in ModInfo()");
		return;
	}

	int32_t modId = modInfo.modid();

	X_LOG1("AssetServer", "ModInfo: id: %" PRIi32, modId);

	{
		core::CriticalSection::ScopedLock slock(lock_);

		assetDb::AssetDB::Mod mod;
		if (db_.GetModInfo(modId, mod))
		{
			response.set_error("");
			response.set_result(ProtoBuf::AssetDB::Result::OK);
			response.set_modid(mod.modId);
			response.set_name(mod.name);
			response.set_path(mod.outDir.c_str());
		}
		else
		{
			response.set_error("Not found");
			response.set_result(ProtoBuf::AssetDB::Result::NOT_FOUND);
		}
	}

	if (!WriteDelimitedTo(response, outputBuffer)) {
		writeError(response, outputBuffer, "Failed to create response msg");
		return;
	}
}

void AssetServer::AssetExsists(const ProtoBuf::AssetDB::AssetExists& exists, ResponseBuffer& outputBuffer)
{
	AssetType::Enum type;
	core::string name = fromStdString(exists.name());

	ProtoBuf::AssetDB::AssetInfoResponse response;
	response.set_name(name);

	if (!MapAssetType(exists.type(), type)) {
		writeError(response, outputBuffer, "Unknown asset type in AssetExsists()");
		return;
	}

	response.set_type(exists.type());

	X_LOG1("AssetServer", "AssetExsists: \"%s\" name: \"%s\"", AssetType::ToString(type), name.c_str());

	{
		core::CriticalSection::ScopedLock slock(lock_);

		int32_t id, modId;
		if (db_.AssetExsists(type, name, &id, &modId))
		{
			response.set_id(id);
			response.set_modid(modId);
			response.set_error("");
			response.set_result(ProtoBuf::AssetDB::Result::OK);
		}
		else
		{
			response.set_error("Not found");
			response.set_result(ProtoBuf::AssetDB::Result::NOT_FOUND);
		}
	}

	if (!WriteDelimitedTo(response, outputBuffer)) {
		writeError(response, outputBuffer, "Failed to create response msg");
		return;
	}
}

void AssetServer::AddAsset(const ProtoBuf::AssetDB::AddAsset& add, ResponseBuffer& outputBuffer)
{
	ProtoBuf::AssetDB::Reponse response;
	AssetType::Enum type;

	if (!MapAssetType(add.type(), type)) {
		writeError(response, outputBuffer, "Unknown asset type in AddAsset()");
		return;
	}
	
	core::string name = fromStdString(add.name());

	X_LOG1("AssetServer", "AddAsset: \"%s\" name: \"%s\"", AssetType::ToString(type), name.c_str());

	core::CriticalSection::ScopedLock slock(lock_);

	auto res = db_.AddAsset(type, name);
	writeResponse(response, outputBuffer, res);
}

void AssetServer::DeleteAsset(const ProtoBuf::AssetDB::DeleteAsset& del, ResponseBuffer& outputBuffer)
{
	ProtoBuf::AssetDB::Reponse response;
	AssetType::Enum type;

	if (!MapAssetType(del.type(), type)) {
		writeError(response, outputBuffer, "Unknown asset type in DeleteAsset()");
		return;
	}

	core::string name = fromStdString(del.name());

	X_LOG1("AssetServer", "DeleteAsset: \"%s\" name: \"%s\"", AssetType::ToString(type), name.c_str());

	core::CriticalSection::ScopedLock slock(lock_);

	auto res = db_.DeleteAsset(type, name);
	writeResponse(response, outputBuffer, res);
}


void AssetServer::RenameAsset(const ProtoBuf::AssetDB::RenameAsset& rename, ResponseBuffer& outputBuffer)
{
	ProtoBuf::AssetDB::Reponse response;
	AssetType::Enum type;

	if (!MapAssetType(rename.type(), type)) {
		writeError(response, outputBuffer, "Unknown asset type in RenameAsset()");
		return;
	}

	core::string name = fromStdString(rename.name());
	core::string newName = fromStdString(rename.newname());

	X_LOG1("AssetServer", "RenameAsset: \"%s\" name: \"%s\" newName: \"%s\"",
		AssetType::ToString(type), name.c_str(), newName.c_str());

	core::CriticalSection::ScopedLock slock(lock_);

	auto res = db_.RenameAsset(type, name, newName);
	writeResponse(response, outputBuffer, res);
}

void AssetServer::UpdateAsset(const ProtoBuf::AssetDB::UpdateAsset& update, core::Array<uint8_t>& data,
	ResponseBuffer& outputBuffer)
{
	ProtoBuf::AssetDB::Reponse response;
	AssetType::Enum type;

	if (!MapAssetType(update.type(), type)) {
		writeError(response, outputBuffer, "Unknown asset type in UpdateAsset()");
		return;
	}

	core::string name = fromStdString(update.name());
	core::string args;
	if (update.has_args()) {
		args = fromStdString(update.args());
	}

	X_LOG1("AssetServer", "UpdateAsset: \"%s\" name: \"%s\"", AssetType::ToString(type), name.c_str());

	core::CriticalSection::ScopedLock slock(lock_);

	auto res = db_.UpdateAsset(type, name, data, args);
	writeResponse(response, outputBuffer, res);
}

template<typename MessageT>
void AssetServer::writeError(MessageT& response, ResponseBuffer& outputBuffer, const char* pMsg)
{
	X_ERROR("Assetserver", "Error: %s", pMsg);

	response.set_result(ProtoBuf::AssetDB::Result::ERROR);
	response.set_error(pMsg);

	if (!WriteDelimitedTo(response, outputBuffer))
	{
		X_ERROR("Assetserver", "Failed to write error response");
	}
}

template<typename MessageT>
void AssetServer::writeResponse(MessageT& response, ResponseBuffer& outputBuffer, assetDb::AssetDB::Result::Enum res)
{
	response.set_result(MapReturnType(res));
	response.set_error("");
	
	if (!WriteDelimitedTo(response, outputBuffer))
	{
		X_ERROR("Assetserver", "Failed to write error response");
	}
}

X_NAMESPACE_END
