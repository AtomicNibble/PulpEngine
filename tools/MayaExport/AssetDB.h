#pragma once

#include <maya\MPxCommand.h>


class AssetDB
{
public:
	X_DECLARE_ENUM(AssetType)(MODEL, ANIM);

public:
	AssetDB();
	~AssetDB();

	static void Init(void);
	static void ShutDown(void);

	MStatus AddAsset(AssetType::Enum type, const MString& name);
	MStatus RemoveAsset(AssetType::Enum type, const MString& name);
	MStatus RenameAsset(AssetType::Enum type, const MString& name, const MString& oldName);

private:

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