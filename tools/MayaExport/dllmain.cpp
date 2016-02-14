#include "stdafx.h"

#include <maya/MObject.h>
#include <maya/MFnPlugin.h>

#include "ModelExporter.h"
#include "AnimExport.h"
#include "SettingsCmd.h"
#include "AssetDB.h"
#include "MayaUtil.h"
#include "EngineApp.h"

#include <IModel.h>
#include <IAnimation.h>

#include <Memory\BoundsCheckingPolicies\NoBoundsChecking.h>
#include <Memory\MemoryTaggingPolicies\NoMemoryTagging.h>
#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>

#include <Memory\BoundsCheckingPolicies\SimpleBoundsChecking.h>
#include <Memory\MemoryTaggingPolicies\SimpleMemoryTagging.h>
#include <Memory\MemoryTrackingPolicies\SimpleMemoryTracking.h>




#if 1
typedef core::MemoryArena<core::MallocFreeAllocator, core::SingleThreadPolicy,
	core::NoBoundsChecking, core::NoMemoryTracking, core::NoMemoryTagging> Arena;
#else
typedef core::MemoryArena<core::MallocFreeAllocator, core::SingleThreadPolicy,
	core::SimpleBoundsChecking, core::SimpleMemoryTracking, core::SimpleMemoryTagging> Arena;
#endif


char* g_createUiScript = "poatoCreateUI";
char* g_destroyUiScript = "poatoDestroyUI";

namespace
{
	core::MallocFreeAllocator g_allocator;

	EngineApp app;
}

core::MemoryArenaBase* g_arena = nullptr;

MODELEX_EXPORT MStatus initializePlugin(MObject obj)
{
	core::StackString<64> ver;
	ver.appendFmt("1.1.0.1 ", model::MODEL_VERSION);
	ver.appendFmt("(m:%i:%i) ", model::MODEL_VERSION, model::MODEL_RAW_VERSION);
	ver.appendFmt("(a:%i:%i)", anim::ANIM_VERSION, anim::ANIM_INTER_VERSION);

	g_arena = new Arena(&g_allocator, "ModelExporterArena");

#if X_DEBUG
	ver.append(" - Debug");
#endif // !X_DEBUG

	MFnPlugin plugin(obj, "WinCat - " X_ENGINE_NAME " Engine", ver.c_str(), "Any");

	MStatus stat;

	MayaUtil::MayaPrintMsg("=== Potato Plugin Loaded (%s) ===", ver.c_str());

	if (!app.Init()) {
		MayaUtil::MayaPrintError("Failed to start engine core");
		return MS::kFailure;
	}

	AssetDB::Init();
	SettingsCache::Init();

	plugin.registerUI(g_createUiScript, g_destroyUiScript);

	stat = plugin.registerCommand("PotatoExportModel",ModelExporterCmd::creator);

	if (stat != MS::kSuccess) {
		stat.perror("Error - initializePlugin");
		return stat;
	}

	stat = plugin.registerCommand("PotatoExportAnim", AnimExporterCmd::creator);
	
	// path util
	plugin.registerCommand("PotatoSettings", SettingsCmd::creator, SettingsCmd::newSyntax);
	plugin.registerCommand("PotatoAssetDB", AssetDBCmd::creator, AssetDBCmd::newSyntax);

	// PotatoPath -get -path_id animout
	if (stat != MS::kSuccess) {
		stat.perror("Error - initializePlugin");
	}

	return stat;
}


MODELEX_EXPORT MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin plugin(obj);

	// an error code
	MStatus stat;

	stat = plugin.deregisterCommand("PotatoExportModel");

	if (stat != MS::kSuccess) {
		stat.perror("Error - uninitializePlugin:PotatoExportModel");
		// should i return or try next :S
	}

	stat = plugin.deregisterCommand("PotatoExportAnim");

	if (stat != MS::kSuccess) {
		stat.perror("Error - uninitializePlugin:PotatoExportAnim");
	}

	stat = plugin.deregisterCommand("PotatoSettings");

	if (stat != MS::kSuccess) {
		stat.perror("Error - uninitializePlugin:PotatoSettings");
	}

	stat = plugin.deregisterCommand("PotatoAssetDB");

	if (stat != MS::kSuccess) {
		stat.perror("Error - uninitializePlugin:PotatoAssetDB");
	}

	AssetDB::ShutDown();
	SettingsCache::ShutDown();

	delete g_arena;
	g_arena = nullptr;

	return stat;
}

