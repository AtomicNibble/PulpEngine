#include "AssetScript.h"
#include "AssetScriptTypes.h"

#include "assetscript\scriptstdstring.h"
#include "assetscript\scriptbuilder.h"


X_NAMESPACE_BEGIN(assman)

namespace
{


} // namepace 


AssetScript::AssetScript()
{

}

AssetScript::~AssetScript()
{
	shutdown();
}

bool AssetScript::init(void)
{
	int32_t r;

	pEngine_ = asCreateScriptEngine();
	r = pEngine_->SetMessageCallback(asFUNCTION(messageCallback), 0, asCALL_CDECL);  BUG_CHECK(r >= 0);

	// stringy tingy..
	RegisterStdString(pEngine_);

	// we will want to register a interface for the script to performs it's shit.
	// we should just pass in a object.
	// that then registers gui items.
	// What gui items :)
	//
	//	combo, checkbox, int edit, float edit, color, vec2, vec3, vec4, text
	//  groups?
	//
	// each item should hae display text and the argument it map's to.
	//
	// How to make the objects, do I have one type that I return for each.
	// or do i do inheritence.
	// or one object that stores info for them all.
	//

	// assetProp
	r = pEngine_->RegisterObjectType("assetProp", sizeof(AssetProperty), asOBJ_REF);  BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("assetProp", asBEHAVE_FACTORY, "assetProp@ f()", asFUNCTION(AssetProperty::factory), asCALL_CDECL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("assetProp", asBEHAVE_FACTORY, "assetProp@ f(const assetProp& in)", asFUNCTION(AssetProperty::copyFactory), asCALL_CDECL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("assetProp", asBEHAVE_ADDREF, "void f()", asMETHOD(AssetProperty, addRef), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("assetProp", asBEHAVE_RELEASE, "void f()", asMETHOD(AssetProperty, release), asCALL_THISCALL); BUG_CHECK(r >= 0);

	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp &opAssign(const assetProp &in)", asMETHODPR(AssetProperty, operator=, (const AssetProperty&), AssetProperty&), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetTitle(const string &in)", asMETHOD(AssetProperty, SetTitle), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetEnabled(bool)", asMETHOD(AssetProperty, SetEnabled), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetVisible(bool)", asMETHOD(AssetProperty, SetVisible), asCALL_THISCALL); BUG_CHECK(r >= 0);

	// asset
	r = pEngine_->RegisterObjectType("asset", sizeof(Asset), asOBJ_REF); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("asset", asBEHAVE_FACTORY, "asset@ f()", asFUNCTION(Asset::factory), asCALL_CDECL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("asset", asBEHAVE_FACTORY, "asset@ f(const asset& in)", asFUNCTION(Asset::copyFactory), asCALL_CDECL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("asset", asBEHAVE_ADDREF, "void f()", asMETHOD(Asset, addRef), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("asset", asBEHAVE_RELEASE, "void f()", asMETHOD(Asset, release), asCALL_THISCALL); BUG_CHECK(r >= 0);
	
	r = pEngine_->RegisterObjectMethod("asset", "asset &opAssign(const asset &in)", asMETHODPR(Asset, operator=, (const Asset&), Asset&), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddTexture(const string& in, const string& in)", asMETHOD(Asset, AddTexture), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddCombo(const string& in, const string)", asMETHOD(Asset, AddCombo), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddCheckBox(const string& in, bool)", asMETHOD(Asset, AddCheckBox), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddInt(const string& in, uint, uint, uint)", asMETHOD(Asset, AddInt), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddFloat(const string& in, float, float, float)", asMETHOD(Asset, AddFloat), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "void BeginGroup(const string& in)", asMETHOD(Asset, BeginGroup), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "void EndGroup(const string& in)", asMETHOD(Asset, EndGroup), asCALL_THISCALL); BUG_CHECK(r >= 0);


	return true;
}

void AssetScript::shutdown(void)
{
	if (pEngine_) {
		pEngine_->ShutDownAndRelease();
		pEngine_ = nullptr;
	}
}


bool AssetScript::processScript(const char* pFileName)
{
	X_ASSERT_NOT_NULL(pEngine_);


	CScriptBuilder builder;
	int32_t r = builder.StartNewModule(pEngine_, "AssetScriptModule");
	if (r < 0)
	{
		X_FATAL("AssetScript", "Unrecoverable error while starting a new module.");
		return false;
	}

	r = builder.AddSectionFromFile(pFileName);
	if (r < 0)
	{
		X_ERROR("AssetScript", "Please correct the errors in the script and try again.");
		return false;
	}
	r = builder.BuildModule();
	if (r < 0)
	{
		X_ERROR("AssetScript", "Please correct the errors in the script and try again.");
		return false;
	}

	// Find the function that is to be called. 
	asIScriptModule* pMod = pEngine_->GetModule("AssetScriptModule");
	asIScriptFunction* pFunc = pMod->GetFunctionByDecl("void GenerateUI(asset& Asset)");
	if (!pFunc)
	{
		X_ERROR("AssetScript", "The script must have the function 'void GenerateUI(asset& ass)'. Please add it and try again.");
		return false;
	}

	Asset asset;

	asIScriptContext* pCtx = pEngine_->CreateContext();
	pCtx->Prepare(pFunc);
	pCtx->SetArgObject(0, &asset);

	r = pCtx->Execute();
	if (r != asEXECUTION_FINISHED)
	{
		X_ERROR("", "Failed to execute assetscript: %" PRIu32, r);

		if (r == asEXECUTION_EXCEPTION)
		{
			printExceptionInfo(pCtx);
		}

		return false;
	}
	else
	{
	}


	return true;
}


void AssetScript::messageCallback(const asSMessageInfo* msg, void* param)
{
	X_UNUSED(param);

	if (msg->type == asMSGTYPE_ERROR)
	{
		X_ERROR("AssetScript", "%s (%d, %d) : %s", msg->section, msg->row, msg->col, msg->message);
	}
	else if (msg->type == asMSGTYPE_WARNING)
	{
		X_WARNING("AssetScript", "%s (%d, %d) : %s", msg->section, msg->row, msg->col, msg->message);
	}
	else if (msg->type == asMSGTYPE_INFORMATION)
	{
		X_LOG0("AssetScript", "%s (%d, %d) : %s", msg->section, msg->row, msg->col, msg->message);
	}
}

void AssetScript::printExceptionInfo(asIScriptContext * pCtx)
{
	const asIScriptFunction *function = pCtx->GetExceptionFunction();

	X_ERROR("AssetScript", "An exception occurred. Please correct the code and try again.");
	X_ERROR("AssetScript", "desc: %s", pCtx->GetExceptionString());
	X_ERROR("AssetScript", "func: %s", function->GetDeclaration());
	X_ERROR("AssetScript", "modl: %s", function->GetModuleName());
	X_ERROR("AssetScript", "sect: %s", function->GetScriptSectionName());
	X_ERROR("AssetScript", "line: %d", pCtx->GetExceptionLineNumber());
}

X_NAMESPACE_END