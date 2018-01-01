#include "stdafx.h"
#include "AssetDB.h"
#include "MayaUtil.h"

#include <Containers\Array.h>
#include <Hashing\Fnva1Hash.h>

#include <maya\MSyntax.h>
#include <maya\MArgDatabase.h>


X_DISABLE_WARNING(4244)
X_DISABLE_WARNING(4100)
#include "..\protobuf\src\assetdb.pb.h"

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
X_ENABLE_WARNING(4244)
X_ENABLE_WARNING(4100)

#if X_DEBUG
X_LINK_LIB("libprotobufd")
#else
X_LINK_LIB("libprotobuf")
#endif // !X_DEBUG


X_NAMESPACE_BEGIN(maya)


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


	ProtoBuf::AssetDB::AssetType AssetTypeToProtoType(AssetDB::AssetType::Enum type)
	{
		switch (type)
		{
		case AssetDB::AssetType::ANIM:
			return ProtoBuf::AssetDB::ANIM;
		case AssetDB::AssetType::MODEL:
			return ProtoBuf::AssetDB::MODEL;
		case AssetDB::AssetType::MATERIAL:
			return ProtoBuf::AssetDB::MATERIAL;
		case AssetDB::AssetType::IMG:
			return ProtoBuf::AssetDB::IMG;
		default:
			break;
		}
		// you STUPID TWAT!
		X_ASSERT_UNREACHABLE();
		return ProtoBuf::AssetDB::ANIM;
	}


	AssetDB* gAssetDb = nullptr;

} // namespace 

AssetDB::AssetDB()
{


}

AssetDB::~AssetDB()
{

}

void AssetDB::Init(void)
{
	if (gAssetDb) {
		return;
	}

	gAssetDb = X_NEW(AssetDB, g_arena, "AssetDB");
	// don't matter if we connect or not still launch.
	gAssetDb->Connect();
}

void AssetDB::ShutDown(void)
{
	if (g_arena) {
		X_DELETE_AND_NULL(gAssetDb, g_arena);
	}
}

AssetDB* AssetDB::Get(void)
{
	X_ASSERT_NOT_NULL(gAssetDb);
	return gAssetDb;
}

bool AssetDB::Connect(void)
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	pipe_.close();

	if (!pipe_.open("\\\\.\\pipe\\" X_ENGINE_NAME "_AssetServer",
		core::IPC::Pipe::OpenMode::READ | core::IPC::Pipe::OpenMode::WRITE |
		core::IPC::Pipe::OpenMode::SHARE
		))
	{
		X_ERROR("AssetDB", "Failed to connect to AssetServer");
		return false;
	}

	return true;
}

bool AssetDB::GetConverterInfo(ConverterInfo& infoOut)
{
	if (!ensureConnected()) {
		MayaUtil::MayaPrintError("Failed to 'GetModInfo' pipe is invalid");
		return false;
	}

	{
		ProtoBuf::AssetDB::ConverterInfoReqest* pRequest = new ProtoBuf::AssetDB::ConverterInfoReqest();

		ProtoBuf::AssetDB::Request request;
		request.set_allocated_coninfo(pRequest);

		if (!sendRequest(request)) {
			return false;
		}
	}

	ProtoBuf::AssetDB::ConverterInfoResponse response;
	if (!readMessage(response)) {
		return false;
	}

	if (response.result() == ProtoBuf::AssetDB::Result::ERROR) {
		const std::string& err = response.error();
		X_ERROR("AssetDB", "Request failed: %s", err.c_str());
		return false;
	}

	infoOut.workingDir.set(response.workingdir().c_str());
	return true;
}


bool AssetDB::GetModInfo(int32_t id, Mod& modOut)
{
	if (!ensureConnected()) {
		MayaUtil::MayaPrintError("Failed to 'GetModInfo' pipe is invalid");
		return false;
	}

	{
		ProtoBuf::AssetDB::ModInfo* pModInfo = new ProtoBuf::AssetDB::ModInfo();
		pModInfo->set_modid(id);

		ProtoBuf::AssetDB::Request request;
		request.set_allocated_modinfo(pModInfo);

		if (!sendRequest(request)) {
			return false;
		}
	}

	ProtoBuf::AssetDB::ModInfoResponse response;
	if (!readMessage(response)) {
		return false;
	}

	if (response.result() == ProtoBuf::AssetDB::Result::ERROR) {
		const std::string& err = response.error();
		X_ERROR("AssetDB", "Request failed: %s", err.c_str());
		return false;
	}

	modOut.modId = response.modid();

	if (response.has_name()) {
		auto& name = response.name();
		modOut.name = core::string(name.data(), name.length());
	} 

	if (response.has_name()) {
		auto& path = response.path();
		modOut.outDir = core::Path<char>(path.data(), path.data() + path.length());
	}
	return true;
}


MStatus AssetDB::AssetExsists(AssetType::Enum type, const MString& name, int32_t* pIdOut, int32_t* pModIdOut)
{
	X_ASSERT_NOT_NULL(pIdOut);
	X_ASSERT_NOT_NULL(pModIdOut);

	*pIdOut = assetDb::INVALID_ASSET_ID;
	*pModIdOut = assetDb::INVALID_MOD_ID;

	if (!ensureConnected()) {
		MayaUtil::MayaPrintError("Failed to 'AssetExsists' pipe is invalid");
		return MS::kFailure;
	}

	{
		ProtoBuf::AssetDB::AssetExists* pExists = new ProtoBuf::AssetDB::AssetExists();
		pExists->set_type(AssetTypeToProtoType(type));
		pExists->set_name(name.asChar());

		ProtoBuf::AssetDB::Request request;
		request.set_allocated_exists(pExists);

		if (!sendRequest(request)) {
			return MS::kFailure;
		}
	}

	ProtoBuf::AssetDB::AssetInfoResponse response;
	if (!readMessage(response)) {
		return MS::kFailure;
	}

	if (response.result() == ProtoBuf::AssetDB::Result::ERROR) {
		const std::string& err = response.error();
		X_ERROR("AssetDB", "Request failed: %s", err.c_str());
		return MS::kFailure;
	}

	if (response.has_id()) {
		*pIdOut = response.id();
	}
	if (response.has_modid()) {
		*pModIdOut = response.modid();
	}
	return MS::kSuccess;
}

MStatus AssetDB::AddAsset(AssetType::Enum type, const MString& name)
{
	if (!ensureConnected()) {
		MayaUtil::MayaPrintError("Failed to 'AddAsset' pipe is invalid");
		return MS::kFailure;
	}

	{
		// YEeeeeEEeeeEEee foooking WUT!
		ProtoBuf::AssetDB::AddAsset* pAdd = new ProtoBuf::AssetDB::AddAsset();
		pAdd->set_type(AssetTypeToProtoType(type));
		pAdd->set_name(name.asChar());

		ProtoBuf::AssetDB::Request request;
		request.set_allocated_add(pAdd);

		if (!sendRequest(request)) {
			return MS::kFailure;
		}
	}
	
	ProtoBuf::AssetDB::Reponse response;

	if (!getResponse(response)) {
		// well fuck.
		return MS::kFailure;
	}

	return MS::kSuccess;
}

MStatus AssetDB::RemoveAsset(AssetType::Enum type, const MString& name)
{
	if (!ensureConnected()) {
		MayaUtil::MayaPrintError("Failed to 'RemoveAsset' pipe is invalid");
		return MS::kFailure;
	}

	{
		ProtoBuf::AssetDB::DeleteAsset* pDel = new ProtoBuf::AssetDB::DeleteAsset();
		pDel->set_type(AssetTypeToProtoType(type));
		pDel->set_name(name.asChar());

		ProtoBuf::AssetDB::Request request;
		request.set_allocated_del(pDel);

		if (!sendRequest(request)) {
			return MS::kFailure;
		}
	}

	ProtoBuf::AssetDB::Reponse response;

	if (!getResponse(response)) {
		return MS::kFailure;
	}

	return MS::kSuccess;
}

MStatus AssetDB::RenameAsset(AssetType::Enum type, const MString& name, const MString& oldName)
{
	if (!ensureConnected()) {
		MayaUtil::MayaPrintError("Failed to 'RenameAsset' pipe is invalid");
		return MS::kFailure;
	}

	{
		ProtoBuf::AssetDB::RenameAsset* pRename = new ProtoBuf::AssetDB::RenameAsset();
		pRename->set_type(AssetTypeToProtoType(type));
		pRename->set_name(oldName.asChar());
		pRename->set_newname(name.asChar());

		ProtoBuf::AssetDB::Request request;
		request.set_allocated_rename(pRename);

		if (!sendRequest(request)) {
			return MS::kFailure;
		}
	}

	ProtoBuf::AssetDB::Reponse response;
	if (!getResponse(response)) {
		return MS::kFailure;
	}

	return MS::kSuccess;
}

MStatus AssetDB::UpdateAsset(AssetType::Enum type, const MString& name, const DataArr& data, bool* pUnchanged)
{
	if (!ensureConnected()) {
		MayaUtil::MayaPrintError("Failed to 'UpdateAsset' pipe is invalid");
		return MS::kFailure;
	}

	{
		uint32_t dataSize = safe_static_cast<uint32_t>(data.size());

		ProtoBuf::AssetDB::UpdateAsset* pUpdate = new ProtoBuf::AssetDB::UpdateAsset();
		pUpdate->set_type(AssetTypeToProtoType(type));
		pUpdate->set_name(name.asChar());
		pUpdate->set_datasize(dataSize);

		ProtoBuf::AssetDB::Request request;
		request.set_allocated_update(pUpdate);

		if (!sendRequest(request)) {
			return MS::kFailure;
		}

		// now send the buffer.
		if (!sendBuf(data)) {
			return MS::kFailure;
		}
	}

	ProtoBuf::AssetDB::Reponse response;

	if (!getResponse(response)) {
		return MS::kFailure;
	}

	if (ProtoBuf::AssetDB::Result::UNCHANGED == response.result() && pUnchanged) {
		*pUnchanged = true;
	}

	return MS::kSuccess;
}

MStatus AssetDB::UpdateAsset(AssetType::Enum type, const MString& name, 
	const MString& args, const DataArr& data, bool* pUnchanged)
{
	if (!ensureConnected()) {
		MayaUtil::MayaPrintError("Failed to 'UpdateAsset' pipe is invalid");
		return MS::kFailure;
	}

	{
		uint32_t dataSize = safe_static_cast<uint32_t>(data.size());

		ProtoBuf::AssetDB::UpdateAsset* pUpdate = new ProtoBuf::AssetDB::UpdateAsset();
		pUpdate->set_type(AssetTypeToProtoType(type));
		pUpdate->set_name(name.asChar());
		pUpdate->set_args(args.asChar());
		pUpdate->set_datasize(dataSize);

		ProtoBuf::AssetDB::Request request;
		request.set_allocated_update(pUpdate);

		if (!sendRequest(request)) {
			return MS::kFailure;
		}

		// now send the buffer.
		if (!sendBuf(data)) {
			return MS::kFailure;
		}
	}

	ProtoBuf::AssetDB::Reponse response;

	if (!getResponse(response)) {
		return MS::kFailure;
	}

	if (ProtoBuf::AssetDB::Result::UNCHANGED == response.result() && pUnchanged) {
		*pUnchanged = true;
	}

	return MS::kSuccess;
}

bool AssetDB::sendRequest(ProtoBuf::AssetDB::Request& request)
{
	uint8_t buffer[BUF_LENGTH];

	google::protobuf::io::ArrayOutputStream arrayOutput(buffer, BUF_LENGTH);
	WriteDelimitedTo(request, &arrayOutput);

	bool firstAttempt = true;

rety:

	if (!pipe_.write(buffer, safe_static_cast<size_t>(arrayOutput.ByteCount()))) {
		X_ERROR("AssetDB", "failed to write buffer");
		pipe_.close();

		// if first try and we can re-connect, try send again.
		if (firstAttempt && Connect()) {
			firstAttempt = false;
			goto rety;
		}
		return false;
	}
	if (!pipe_.flush()) {
		X_ERROR("AssetDB", "failed to flush pipe");
		return false;
	}

	return true;
}

bool AssetDB::sendBuf(const DataArr& data)
{
	size_t bytesWritten;
	size_t totalBytesWritten = 0;

	while (1)
	{
		const size_t bytesLeft = data.size() - totalBytesWritten;

		bytesWritten = 0;
		if (!pipe_.write(&data[bytesWritten], bytesLeft, &bytesWritten)) {
			X_ERROR("AssetDB", "failed to write buffer");
			pipe_.close();

			// don't think we wan't to rety in this case.
			return false;
		}

		totalBytesWritten += bytesWritten;
		if (totalBytesWritten >= data.size()) {
			break;
		}
	}

	if (!pipe_.flush()) {
		X_ERROR("AssetDB", "failed to flush pipe");
		return false;
	}

	return true;
}

bool AssetDB::getResponse(ProtoBuf::AssetDB::Reponse& response)
{
	if (!readMessage(response)) {
		X_ERROR("AssetDB", "Failed to read response msg");
		return false;
	}

	if (response.result() == ProtoBuf::AssetDB::Result::ERROR) {
		const std::string& err = response.error();
		X_ERROR("AssetDB", "Request failed: %s", err.c_str());
		return false;
	}

	return true;
}

bool AssetDB::readMessage(google::protobuf::MessageLite& message)
{
	uint8_t buffer[BUF_LENGTH];
	size_t bytesRead;
	bool cleanEof;

	if (!pipe_.read(buffer, sizeof(buffer), &bytesRead)) {
		X_ERROR("AssetDB", "failed to read response");
		pipe_.close(); // close it so we can open a new one
		return false;
	}

	google::protobuf::io::ArrayInputStream arrayInput(buffer,
		safe_static_cast<int32_t>(bytesRead));

	if (!ReadDelimitedFrom(&arrayInput, &message, &cleanEof)) {
		X_ERROR("AssetDB", "Failed to read response msg");
		pipe_.close(); // close it so we can open a new one
		return false;
	}

	return true;
}

bool AssetDB::ensureConnected(void)
{
	if (pipe_.isOpen()) {
		return true;
	}

	return Connect();
}

// ------------------------------------------------------------------

AssetDBCmd::AssetDBCmd()
{
}

AssetDBCmd::~AssetDBCmd()
{
}

MStatus AssetDBCmd::doIt(const MArgList &args)
{
	MStatus stat;
	MArgDatabase parser(syntax(), args, &stat);

	if (stat != MS::kSuccess) {
		return stat;
	}

	AssetType::Enum assetType;
	Action::Enum action;

	MString name, oldName;

	if (parser.isFlagSet("-a")) 
	{
		MString actionStr;

		if (!parser.getFlagArgument("-a", 0, actionStr)) {
			setResult("failed to get argument: -action(-a)");
			return MS::kSuccess;
		}

		// work out action type.
		// is it a valid path id?
		if (core::strUtil::IsEqualCaseInsen(actionStr.asChar(), "add")) {
			action = Action::ADD;
		}
		else if (core::strUtil::IsEqualCaseInsen(actionStr.asChar(), "remove")) {
			action = Action::REMOVE;
		}
		else if (core::strUtil::IsEqualCaseInsen(actionStr.asChar(), "rename")) {
			action = Action::RENAME;
		}
		else {
			core::StackString512 msg;
			msg.appendFmt("unknown action: '%s' valid action's: add, remove, rename", actionStr.asChar());
			setResult(msg.c_str());
			return MS::kSuccess;
		}
	}	
	else
	{
		setResult("missing required argument: -action(-a)");
		return MS::kSuccess;
	}

	if (parser.isFlagSet("-t"))
	{
		MString typeStr;

		if (!parser.getFlagArgument("-t", 0, typeStr)) {
			setResult("failed to get argument: -type(-t)");
			return MS::kSuccess;
		}

		// work out action type.
		// is it a valid path id?
		static_assert(AssetType::ENUM_COUNT == 16, "More asset types :[] ? this code needs updating.");

		typeStr.toLowerCase();

		using namespace core::Hash::Fnv1Literals;
		switch (core::Hash::Fnv1aHash(typeStr.asChar(), typeStr.length()))
		{
			case "model"_fnv1a:
				assetType = AssetType::MODEL;
				break;
			case "anim"_fnv1a:
				assetType = AssetType::ANIM;
				break;
			case "material"_fnv1a:
				assetType = AssetType::MATERIAL;
				break;
			case "img"_fnv1a:
				assetType = AssetType::IMG;
				break;
			case "weapon"_fnv1a:
				assetType = AssetType::WEAPON;
				break;
			case "turret"_fnv1a:
				assetType = AssetType::TURRET;
				break;
			case "light"_fnv1a:
				assetType = AssetType::LIGHT;
				break;
			case "fx"_fnv1a:
				assetType = AssetType::FX;
				break;
			case "rumble"_fnv1a:
				assetType = AssetType::RUMBLE;
				break;
			case "shellshock"_fnv1a:
				assetType = AssetType::SHELLSHOCK;
				break;
			case "character"_fnv1a:
				assetType = AssetType::CHARACTER;
				break;
			case "vehicle"_fnv1a:
				assetType = AssetType::VEHICLE;
				break;
			case "camera"_fnv1a:
				assetType = AssetType::CAMERA;
				break;
			case "video"_fnv1a:
				assetType = AssetType::VIDEO;
				break;
			case "script"_fnv1a:
				assetType = AssetType::SCRIPT;
				break;
			case "font"_fnv1a:
				assetType = AssetType::FONT;
				break;
			default:
				core::StackString512 msg;
				msg.appendFmt("unknown type: '%s' valid type's: model, anim, material", typeStr.asChar());
				setResult(msg.c_str());
				return MS::kSuccess;
		}
	}
	else
	{
		setResult("missing required argument: -type(-t)");
		return MS::kSuccess;
	}

	if (parser.isFlagSet("-n"))
	{
		if (!parser.getFlagArgument("-n", 0, name)) {
			setResult("failed to get argument: -name(-n)");
			return MS::kSuccess;
		}
	}
	else
	{
		setResult("missing required argument: -name(-n)");
		return MS::kSuccess;
	}

	if (action == Action::RENAME)
	{
		if (parser.isFlagSet("-on"))
		{
			if (!parser.getFlagArgument("-on", 0, oldName)) {
				setResult("failed to get argument: -old_name(-on)");
				return MS::kSuccess;
			}
		}
		else
		{
			setResult("missing required argument: -old_name(-on)");
			return MS::kSuccess;
		}
	}

	if (!gAssetDb) {
		setResult("AssetDB is invalid, Init must be called (source code error)");
		return MS::kSuccess;
	}

	stat = MS::kFailure;

	// all good.
	if (action == Action::ADD)
	{
		stat = gAssetDb->AddAsset(assetType, name);
	}
	else if (action == Action::REMOVE)
	{
		stat = gAssetDb->RemoveAsset(assetType, name);
	}
	else if (action == Action::RENAME)
	{
		stat = gAssetDb->RenameAsset(assetType, name, oldName);
	}
	else
	{
		X_ASSERT_UNREACHABLE();
	}

	if (stat == MS::kSuccess) {
		setResult("ok");
	}
	else {
		setResult("DB action failed, check log for more info");
	}

	// we return kSuccess otherwise setResult is not passed back :(
	return MS::kSuccess;
}

void* AssetDBCmd::creator(void)
{
	return new AssetDBCmd;
}

MSyntax AssetDBCmd::newSyntax(void)
{
	MSyntax syn;

	syn.addFlag("-a", "-action", MSyntax::kString);
	syn.addFlag("-t", "-type", MSyntax::kString);
	syn.addFlag("-n", "-name", MSyntax::kString);
	syn.addFlag("-on", "-old_name", MSyntax::kString);

	return syn;
}

X_NAMESPACE_END