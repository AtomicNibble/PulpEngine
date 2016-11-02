#pragma once

#include <maya\MPxCommand.h>

#include <Containers\Array.h>

#include <Platform\Pipe.h>

X_NAMESPACE_DECLARE(ProtoBuf,
namespace AssetDB {
	class Request;
	class Reponse;
}
);




X_NAMESPACE_BEGIN(maya)

class AssetDB
{
public:
	X_DECLARE_ENUM(AssetType)(MODEL, ANIM, MATERIAL, IMG);

public:
	AssetDB();
	~AssetDB();

	static void Init(void);
	static void ShutDown(void);
	static AssetDB* Get(void);

	bool Connect(void);

	MStatus AddAsset(AssetType::Enum type, const MString& name);
	MStatus RemoveAsset(AssetType::Enum type, const MString& name);
	MStatus RenameAsset(AssetType::Enum type, const MString& name, const MString& oldName);
	MStatus UpdateAsset(AssetType::Enum type, const MString& name, const MString& args, 
		const core::Array<uint8_t>& data, bool* pUnchanged = nullptr);

private:
	bool sendRequest(ProtoBuf::AssetDB::Request& request);
	bool sendBuf(const core::Array<uint8_t>& data);
	bool getResponse(ProtoBuf::AssetDB::Reponse& response);

private:
	core::IPC::Pipe pipe_;
};


class AssetDBCmd : public MPxCommand
{
	X_DECLARE_ENUM(Action)(ADD, REMOVE, RENAME);

	typedef AssetDB::AssetType AssetType;

public:
	AssetDBCmd();
	~AssetDBCmd();

	virtual MStatus doIt(const MArgList &args) X_OVERRIDE;

	static void* creator(void);
	static MSyntax newSyntax(void);

private:
};

X_NAMESPACE_END