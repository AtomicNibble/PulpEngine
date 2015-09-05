#include "stdafx.h"

#include <maya/MObject.h>
#include <maya/MFnPlugin.h>

#include "ModelExporter.h"

#include <IModel.h>
#include <String\StackString.h>

char* g_OptionScript = "PotatoPluginOptions";
char* g_DefaultExportOptions = "";

char* g_createUiScript = "poatoCreateUI";
char* g_destroyUiScript = "poatoDestroyUI";

core::MallocFreeAllocator g_allocator;
Arena g_arena(&g_allocator, "ModelExporterArena");


#define ADD_FILE_TRANS 0

MODELEX_EXPORT MStatus initializePlugin(MObject obj)
{
	core::StackString<64> ver;
	ver.appendFmt("1.0.%i.2", model::MODEL_VERSION);

	MFnPlugin plugin(obj, "WinCat - "X_ENGINE_NAME" Engine", ver.c_str(), "Any");

	MStatus stat;

	plugin.registerUI(g_createUiScript, g_destroyUiScript);

#if ADD_FILE_TRANS
	stat = plugin.registerFileTranslator("Potato_model",
		"none",
		ModelExporter::creator,
		(char*)g_OptionScript,
		(char*)g_DefaultExportOptions);
#endif

	stat = plugin.registerCommand("PotatoExportModel",ModelExporterCmd::creator);

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

	// de-register the file translator with Maya 
#if ADD_FILE_TRANS
	stat = plugin.deregisterFileTranslator("Potato_model");
#endif

	stat = plugin.deregisterCommand("PotatoExportModel");

	if (stat != MS::kSuccess) {
		stat.perror("Error - uninitializePlugin");
	}
	return stat;
}

