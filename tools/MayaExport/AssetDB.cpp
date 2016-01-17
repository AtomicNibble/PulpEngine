#include "stdafx.h"
#include "AssetDB.h"
#include "MayaUtil.h"

#include <maya\MSyntax.h>
#include <maya\MArgDatabase.h>

namespace
{

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
	gAssetDb = X_NEW(AssetDB, g_arena, "AssetDB");
}

void AssetDB::ShutDown(void)
{
	if (g_arena) {
		X_DELETE_AND_NULL(gAssetDb, g_arena);
	}
}


MStatus AssetDB::AddAsset(AssetType::Enum type, const MString & name)
{
	X_UNUSED(type);
	X_UNUSED(name);
	return MStatus();
}

MStatus AssetDB::RemoveAsset(AssetType::Enum type, const MString & name)
{
	X_UNUSED(type);
	X_UNUSED(name);
	return MStatus();
}

MStatus AssetDB::RenameAsset(AssetType::Enum type, const MString & name, const MString & oldName)
{
	X_UNUSED(type);
	X_UNUSED(name);
	X_UNUSED(oldName);
	return MStatus();
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

	setResult(false);

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
			MayaUtil::MayaPrintError("failed to get argument: -action(-a)");
			return MS::kFailure;
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
			MayaUtil::MayaPrintError("unkown action: '%s' valid action's: add, remove, rename", actionStr.asChar());
			return MS::kFailure;
		}
	}	
	else
	{
		MayaUtil::MayaPrintError("missing required argument: -action(-a)");
		return MS::kFailure;
	}

	if (parser.isFlagSet("-t"))
	{
		MString typeStr;

		if (!parser.getFlagArgument("-t", 0, typeStr)) {
			MayaUtil::MayaPrintError("failed to get argument: -type(-t)");
			return MS::kFailure;
		}

		// work out action type.
		// is it a valid path id?
		if (core::strUtil::IsEqualCaseInsen(typeStr.asChar(), "model")) {
			assetType = AssetType::MODEL;
		}
		else if (core::strUtil::IsEqualCaseInsen(typeStr.asChar(), "anim")) {
			assetType = AssetType::ANIM;
		}
		else {
			MayaUtil::MayaPrintError("unkown type: '%s' valid type's: model, anim", typeStr.asChar());
			return MS::kFailure;
		}
	}
	else
	{
		MayaUtil::MayaPrintError("missing required argument: -type(-t)");
		return MS::kFailure;
	}

	if (parser.isFlagSet("-n"))
	{
		if (!parser.getFlagArgument("-n", 0, name)) {
			MayaUtil::MayaPrintError("failed to get argument: -name(-n)");
			return MS::kFailure;
		}
	}
	else
	{
		MayaUtil::MayaPrintError("missing required argument: -name(-n)");
		return MS::kFailure;
	}

	if (action == Action::RENAME)
	{
		if (parser.isFlagSet("-on"))
		{
			if (!parser.getFlagArgument("-on", 0, oldName)) {
				MayaUtil::MayaPrintError("failed to get argument: -old_name(-on)");
				return MS::kFailure;
			}
		}
		else
		{
			MayaUtil::MayaPrintError("missing required argument: -old_name(-on)");
			return MS::kFailure;
		}
	}

	if (!gAssetDb) {
		MayaUtil::MayaPrintError("AssetDB is invalid, Init must be called (source code error)");
		return MS::kFailure;
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
		setResult(true);
	}

	return stat;
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

