#include "AssetScript.h"
#include "AssetScriptTypes.h"

#include "assetscript\scriptstdstring.h"
#include "assetscript\scriptbuilder.h"


X_NAMESPACE_BEGIN(assman)

namespace
{


} // namepace 


const char* AssetPropsScript::ASSET_PROPS_SCRIPT_EXT = "aps";
const char* AssetPropsScript::SCRIPT_ENTRY = "void GenerateUI(asset& AssetProps)";


AssetPropsScript::AssetPropsScript()
{

}

AssetPropsScript::~AssetPropsScript()
{
	shutdown();
}

bool AssetPropsScript::init(void)
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
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetToolTip(const string &in)", asMETHOD(AssetProperty, SetToolTip), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetIcon(const string &in)", asMETHOD(AssetProperty, SetIcon), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetFontColor(float r, float g, float b)", asMETHOD(AssetProperty, SetFontColor), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetBold(bool)", asMETHOD(AssetProperty, SetBold), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetStep(double step)", asMETHOD(AssetProperty, SetStep), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetEnabled(bool)", asMETHOD(AssetProperty, SetEnabled), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetVisible(bool)", asMETHOD(AssetProperty, SetVisible), asCALL_THISCALL); BUG_CHECK(r >= 0);

	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetValue(const assetProp &in)", asMETHOD(AssetProperty, SetValue), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetBool(bool val)", asMETHOD(AssetProperty, SetBool), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetInt(int val)", asMETHOD(AssetProperty, SetInt), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetFloat(float val)", asMETHOD(AssetProperty, SetFloat), asCALL_THISCALL); BUG_CHECK(r >= 0);

	r = pEngine_->RegisterObjectMethod("assetProp", "string GetTitle()", asMETHOD(AssetProperty, GetTitle), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "string GetToolTip()", asMETHOD(AssetProperty, GetToolTip), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "string GetValue()", asMETHOD(AssetProperty, GetValue), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "bool GetBool()", asMETHOD(AssetProperty, GetBool), asCALL_THISCALL); BUG_CHECK(r >= 0);


	// asset
	r = pEngine_->RegisterObjectType("asset", sizeof(AssetProps), asOBJ_REF); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("asset", asBEHAVE_FACTORY, "asset@ f()", asFUNCTION(AssetProps::factory), asCALL_CDECL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("asset", asBEHAVE_FACTORY, "asset@ f(const asset& in)", asFUNCTION(AssetProps::copyFactory), asCALL_CDECL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("asset", asBEHAVE_ADDREF, "void f()", asMETHOD(AssetProps, addRef), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("asset", asBEHAVE_RELEASE, "void f()", asMETHOD(AssetProps, release), asCALL_THISCALL); BUG_CHECK(r >= 0);
	
	r = pEngine_->RegisterObjectMethod("asset", "asset &opAssign(const asset &in)", asMETHODPR(AssetProps, operator=, (const AssetProps&), AssetProps&), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddTexture(const string& in, const string& in)", asMETHOD(AssetProps, AddTexture), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddCombo(const string& in, const string)", asMETHOD(AssetProps, AddCombo), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddCheckBox(const string& in, bool)", asMETHOD(AssetProps, AddCheckBox), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddInt(const string& in, uint, uint, uint)", asMETHOD(AssetProps, AddInt), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddFloat(const string& in, double val, double min, double max)", asMETHOD(AssetProps, AddFloat), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddVec2(const string& in, double x, double y,  double min, double max)", asMETHOD(AssetProps, AddVec2), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddVec3(const string& in, double x, double y, double z, double min, double max)", asMETHOD(AssetProps, AddVec3), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddVec4(const string& in, double x, double y, double z, double w, double min, double max)", asMETHOD(AssetProps, AddVec4), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddText(const string& in, const string& in)", asMETHOD(AssetProps, AddText), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddPath(const string& in, const string& in)", asMETHOD(AssetProps, AddPath), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "void BeginGroup(const string& in)", asMETHOD(AssetProps, BeginGroup), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "void EndGroup(const string& in)", asMETHOD(AssetProps, EndGroup), asCALL_THISCALL); BUG_CHECK(r >= 0);


	return true;
}

void AssetPropsScript::shutdown(void)
{
	if (pEngine_) {
		pEngine_->ShutDownAndRelease();
		pEngine_ = nullptr;
	}

	clearCache();
}


bool AssetPropsScript::getAssetPropsForType(AssetProps& props, assetDb::AssetType::Enum type, bool reload)
{
	if (!reload && loadFromCache(props, type)) {
		return true;
	}

	if (processScriptForType(props, type)) {
		setCache(props, type);
		return true;
	}

	// fallback to cache if present even tho reload was specified?
	// ...

	X_ERROR("AssetScript", "Failed to get assetProps for asset type: \"%s\"", assetDb::AssetType::ToString(type));
	return false;
}

bool AssetPropsScript::loadFromCache(AssetProps& props, assetDb::AssetType::Enum type)
{
	if (cache_[type]) {
		props = *cache_[type];
		return true;
	}
	return false;
}

void AssetPropsScript::setCache(AssetProps& props, assetDb::AssetType::Enum type)
{
	if (!cache_[type]) {
		cache_[type] = new AssetProps(props);
	}
	else {
		*cache_[type] = props;
	}
}

void AssetPropsScript::clearCache(void)
{
	for(auto& pEntry : cache_) {
		if (pEntry) {
			delete pEntry;
			pEntry = nullptr;
		}
	}
}

bool AssetPropsScript::processScriptForType(AssetProps& props, assetDb::AssetType::Enum type)
{
	core::Path<char> path;
	path.append("assetscripts");
	path.append(assetDb::AssetType::ToString(type));
	path.setExtension(ASSET_PROPS_SCRIPT_EXT);

	return processScript(props, path.c_str());
}


bool AssetPropsScript::processScript(AssetProps& props, const char* pFileName)
{
	X_ASSERT_NOT_NULL(pEngine_);

	CScriptBuilder builder;
	int32_t r = builder.StartNewModule(pEngine_, "AssetScriptModule");
	if (r < 0)
	{
		X_FATAL("AssetScript", "Unrecoverable error while starting a new module. Err: %" PRIi32, r);
		return false;
	}

	r = builder.AddSectionFromFile(pFileName);
	if (r < 0)
	{
		X_ERROR("AssetScript", "Please correct the errors in \"%s\" script and try again. Err: %" PRIi32, pFileName, r);
		return false;
	}
	r = builder.BuildModule();
	if (r < 0)
	{
		X_ERROR("AssetScript", "Please correct the errors in \"%s\" script and try again. Err: %" PRIi32, pFileName, r);
		return false;
	}

	// Find the function that is to be called. 
	asIScriptModule* pMod = pEngine_->GetModule("AssetScriptModule");
	X_ASSERT_NOT_NULL(pMod);
	asIScriptFunction* pFunc = pMod->GetFunctionByDecl(SCRIPT_ENTRY);
	if (!pFunc)
	{
		X_ERROR("AssetScript", "The script \"%s\" must have the function 'void GenerateUI(asset& ass)'. Please add it and try again.",
			pFileName, SCRIPT_ENTRY);
		return false;
	}

	asIScriptContext* pCtx = pEngine_->CreateContext();
	pCtx->Prepare(pFunc);
	pCtx->SetArgObject(0, &props);

	r = pCtx->Execute();
	if (r != asEXECUTION_FINISHED)
	{
		X_ERROR("AssetScript", "Failed to execute script \"%s\" Err: %" PRIu32, pFileName, r);

		if (r == asEXECUTION_EXCEPTION) {
			printExceptionInfo(pCtx);
		}

		return false;
	}

	return true;
}


void AssetPropsScript::messageCallback(const asSMessageInfo* msg, void* param)
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
	else
	{
		X_ASSERT_UNREACHABLE();
	}
}

void AssetPropsScript::printExceptionInfo(asIScriptContext * pCtx)
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