#include "stdafx.h"

#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MFnPlugin.h>
#include <maya/MSceneMessage.h>

#include "ModelExporter.h"
#include "AnimExport.h"
#include "SettingsCmd.h"
#include "AssetDB.h"
#include "MayaUtil.h"
#include "EngineApp.h"

#include "MayaLogger.h"
#include "MayaLoggerFormat.h"

#include <IModel.h>
#include <IAnimation.h>

#include <Memory\BoundsCheckingPolicies\NoBoundsChecking.h>
#include <Memory\MemoryTaggingPolicies\NoMemoryTagging.h>
#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>

#include <Memory\BoundsCheckingPolicies\SimpleBoundsChecking.h>
#include <Memory\MemoryTaggingPolicies\SimpleMemoryTagging.h>
#include <Memory\MemoryTrackingPolicies\SimpleMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include "Logging\Logger.h"
#include "Logging\FilterPolicies\LoggerNoFilterPolicy.h"
#include "Logging\FilterPolicies\LoggerVerbosityFilterPolicy.h"
#include "Logging\FormatPolicies\LoggerSimpleFormatPolicy.h"

#include <Time\StopWatch.h>


X_LINK_LIB(X_STRINGIZE(MAYA_SDK) "\\Foundation")
X_LINK_LIB(X_STRINGIZE(MAYA_SDK) "\\OpenMaya")
X_LINK_LIB(X_STRINGIZE(MAYA_SDK) "\\OpenMayaUI")
X_LINK_LIB(X_STRINGIZE(MAYA_SDK) "\\OpenMayaAnim")


#if 1
typedef core::MemoryArena<core::MallocFreeAllocator, core::MultiThreadPolicy<core::Spinlock>,
	core::NoBoundsChecking, core::NoMemoryTracking, core::NoMemoryTagging> Arena;
#else
typedef core::MemoryArena<core::MallocFreeAllocator, core::MultiThreadPolicy<core::Spinlock>,
	core::SimpleBoundsChecking, core::SimpleMemoryTracking, core::SimpleMemoryTagging> Arena;
#endif


const char* g_createUiScript = "poatoCreateUI";
const char* g_destroyUiScript = "poatoDestroyUI";

namespace
{
	core::MallocFreeAllocator g_allocator;

	EngineApp app;

	MCallbackId AfterFileOpenCallBackId;
	MCallbackId AfterNewCallBackId;

	static void AfterFileOpen(void* pClientData)
	{
		X_UNUSED(pClientData);
		MGlobal::executeCommandOnIdle("potatoAfterFileOpen();");
	}

	static void AfterNew(void* pClientData)
	{
		X_UNUSED(pClientData);
		MGlobal::executeCommandOnIdle("potatoAfterNew();");
	}
	

	typedef core::Logger<
		core::LoggerNoFilterPolicy,
		maya::LoggerMayaFormatPolicy,
		maya::LoggerMayaWritePolicy> MayaLogger;

	// this global is removed before engine app does shutdown
	// as the uninitializePlugin is never called :/
	MayaLogger* pLogger;
}

core::MemoryArenaBase* g_arena = nullptr;

X_USING_NAMESPACE;

using namespace maya;

MODELEX_EXPORT MStatus initializePlugin(MObject obj)
{
	core::StopWatch timer;

	core::StackString<64> ver;
	ver.appendFmt("1.1.0.1 ", model::MODEL_VERSION);
	ver.appendFmt("(m:%" PRIu32 ":%" PRIu32 ") ", model::MODEL_VERSION, model::MODEL_RAW_VERSION);
	ver.appendFmt("(a:%" PRIu32 ":%" PRIu32 ")", anim::ANIM_VERSION, anim::ANIM_INTER_VERSION);

	g_arena = new Arena(&g_allocator, "ModelExporterArena");

#if X_DEBUG
	ver.append(" - Debug");
#endif // !X_DEBUG

	MFnPlugin plugin(obj, "WinCat - " X_ENGINE_NAME " Engine", ver.c_str(), "Any");

	MStatus stat;

	if (!app.Init()) {
		MayaUtil::MayaPrintError("Failed to start engine core");
		return MS::kFailure;
	}

	pLogger = X_NEW(MayaLogger, g_arena, "MayaLogger");

	gEnv->pLog->AddLogger(pLogger);

	AssetDB::Init();
	SettingsCache::Init();

	plugin.registerUI(g_createUiScript, g_destroyUiScript);

	stat = plugin.registerCommand("PotatoExportModel",ModelExporterCmd::creator, ModelExporterCmd::newSyntax);

	if (stat != MS::kSuccess) {
		stat.perror("Error - initializePlugin");
		return stat;
	}

	stat = plugin.registerCommand("PotatoExportAnim", AnimExporterCmd::creator);
	
	if (stat != MS::kSuccess) {
		stat.perror("Error - initializePlugin");
		return stat;
	}

	// path util
	stat = plugin.registerCommand("PotatoSettings", SettingsCmd::creator, SettingsCmd::newSyntax);

	if (stat != MS::kSuccess) {
		stat.perror("Error - initializePlugin");
		return stat;
	}

	stat =  plugin.registerCommand("PotatoAssetDB", AssetDBCmd::creator, AssetDBCmd::newSyntax);

	// PotatoPath -get -path_id animout
	if (stat != MS::kSuccess) {
		stat.perror("Error - initializePlugin");
		return stat;
	}

	AfterFileOpenCallBackId = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, AfterFileOpen, nullptr, &stat);
	if (stat != MS::kSuccess) {
		stat.perror("Error - initializePlugin:OnSceneUpdate");
		return stat;
	}

	AfterNewCallBackId = MSceneMessage::addCallback(MSceneMessage::kAfterNew, AfterNew, nullptr, &stat);
	if (stat != MS::kSuccess) {
		stat.perror("Error - initializePlugin:OnSceneUpdate");
		return stat;
	}


	MayaUtil::MayaPrintMsg("=== " X_ENGINE_NAME " Plugin Loaded (%s %gms) ===", ver.c_str(),
		timer.GetMilliSeconds());

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

	// remove the callback
	stat = MMessage::removeCallback(AfterFileOpenCallBackId);
	
	if (stat != MS::kSuccess) {
		stat.perror("Error - uninitializePlugin:removeCallback");
	}


	AssetDB::ShutDown();
	SettingsCache::ShutDown();

	gEnv->pLog->RemoveLogger(pLogger);

	X_DELETE_AND_NULL(pLogger, g_arena);

	delete g_arena;
	g_arena = nullptr;

	return stat;
}
