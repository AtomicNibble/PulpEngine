#include "AssetScript.h"
#include "AssetScriptTypes.h"

#include "assetscript\scriptstdstring.h"
#include "assetscript\scriptbuilder.h"


X_NAMESPACE_BEGIN(assman)

namespace
{


} // namepace 

AssetPropsScript::ByteCodeStream::ByteCodeStream() :
	readPos_(0)
{

}

bool AssetPropsScript::ByteCodeStream::isNotEmpty(void) const
{
	return !data_.empty();
}

void AssetPropsScript::ByteCodeStream::clear(void)
{
	readPos_ = 0;
	data_.clear();
}

void AssetPropsScript::ByteCodeStream::resetRead(void)
{
	readPos_ = 0;
}


void AssetPropsScript::ByteCodeStream::Write(const void *ptr, asUINT size)
{
	if (size == 0) {
		return;
	}

	const size_t writePos = data_.size();
	data_.resize(data_.size() + size);
	std::memcpy(&data_[writePos], ptr, size);
}

void AssetPropsScript::ByteCodeStream::Read(void *ptr, asUINT size)
{
	if (size == 0) {
		return;
	}

	if ((size + readPos_) > data_.size()) {
		size = static_cast<uint32_t>( data_.size() - readPos_);
	}

	std::memcpy(ptr, &data_[readPos_], size);
}

// ----------------------------------------------------------

void AssetPropsScript::Scriptcache::clear(bool byteCodeOnly)
{
	if (!byteCodeOnly) {
		text.clear();
	}
	byteCode.clear();
}

// ----------------------------------------------------------

const char* AssetPropsScript::ASSET_PROPS_SCRIPT_EXT = "aps";
const char* AssetPropsScript::SCRIPT_ENTRY = "void GenerateUI(asset& AssetProps)";


AssetPropsScript::AssetPropsScript() :
	pEngine_(nullptr)
{
	// cache_.fill(nullptr);
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
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetLabels(const string &in, const string &in, const string &in, const string &in)", asMETHOD(AssetProperty, SetLabels), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetIcon(const string &in)", asMETHOD(AssetProperty, SetIcon), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetFontColor(float r, float g, float b)", asMETHOD(AssetProperty, SetFontColor), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetBold(bool)", asMETHOD(AssetProperty, SetBold), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetStep(double step)", asMETHOD(AssetProperty, SetStep), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetEnabled(bool)", asMETHOD(AssetProperty, SetEnabled), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetVisible(bool)", asMETHOD(AssetProperty, SetVisible), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& ShowJumpToAsset(bool)", asMETHOD(AssetProperty, ShowJumpToAsset), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& UpdateOnChange(bool)", asMETHOD(AssetProperty, UpdateOnChange), asCALL_THISCALL); BUG_CHECK(r >= 0);

	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetValue(const string &in)", asMETHOD(AssetProperty, SetValue), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetDefaultValue(const string &in)", asMETHOD(AssetProperty, SetDefaultValue), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetBool(bool val)", asMETHOD(AssetProperty, SetBool), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetInt(int val)", asMETHOD(AssetProperty, SetInt), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetFloat(float val)", asMETHOD(AssetProperty, SetFloat), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp& SetDouble(double val)", asMETHOD(AssetProperty, SetDouble), asCALL_THISCALL); BUG_CHECK(r >= 0);

	r = pEngine_->RegisterObjectMethod("assetProp", "string GetTitle()", asMETHOD(AssetProperty, GetTitle), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "string GetToolTip()", asMETHOD(AssetProperty, GetToolTip), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "string GetValue()", asMETHOD(AssetProperty, GetValue), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "double GetFloat()", asMETHOD(AssetProperty, GetValueFloat), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "int GetInt()", asMETHOD(AssetProperty, GetValueInt), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "bool GetBool()", asMETHOD(AssetProperty, GetValueBool), asCALL_THISCALL); BUG_CHECK(r >= 0);


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
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddVec2(const string& in, const string& in, double x, double y,  double min, double max)", asMETHOD(AssetProps, AddVec2), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddVec3(const string& in, const string& in, const string& in, double x, double y, double z, double min, double max)", asMETHOD(AssetProps, AddVec3), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddVec4(const string& in, const string& in, const string& in, const string& in, double x, double y, double z, double w, double min, double max)", asMETHOD(AssetProps, AddVec4), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddText(const string& in, const string& in)", asMETHOD(AssetProps, AddText), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp& AddPath(const string& in, const string& in)", asMETHOD(AssetProps, AddPath), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "void BeginGroup(const string& in)", asMETHOD(AssetProps, BeginGroup), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "void EndGroup(const string& in)", asMETHOD(AssetProps, EndGroup), asCALL_THISCALL); BUG_CHECK(r >= 0);

	r = pEngine_->RegisterObjectMethod("asset", "assetProp& getItem(const string& in)", asMETHOD(AssetProps, getItem), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "string getPropValue(const string& in)", asMETHOD(AssetProps, getPropValue), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "double getPropValueFloat(const string& in)", asMETHOD(AssetProps, getPropValueFloat), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "int getPropValueInt(const string& in)", asMETHOD(AssetProps, getPropValueInt), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "bool getPropValueBool(const string& in)", asMETHOD(AssetProps, getPropValueBool), asCALL_THISCALL); BUG_CHECK(r >= 0);


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


bool AssetPropsScript::runScriptForProps(AssetProps& props, assetDb::AssetType::Enum type)
{
	if (!ensureSourceCache(type)) {
		X_ERROR("AssetScript", "Failed to get assetProps for asset type: \"%s\"", assetDb::AssetType::ToString(type));
		return false;
	}

	return processScript(props, cache_[type]);
}


void AssetPropsScript::clearCache(bool byteCodeOnly)
{
	for(auto& entry : cache_) {
		entry.clear(byteCodeOnly);
	}
}

void AssetPropsScript::clearCache(assetDb::AssetType::Enum type, bool byteCodeOnly)
{
	cache_[type].clear(byteCodeOnly);
}

bool AssetPropsScript::sourceCacheValid(assetDb::AssetType::Enum type) const
{
	return !cache_[type].text.empty();
}

bool AssetPropsScript::ensureSourceCache(assetDb::AssetType::Enum type, bool reload)
{
	if (!reload && sourceCacheValid(type)) {
		return true;
	}

	bool res = loadScript(type, cache_[type].text);
	if (!res) {
		// make sure it's emopty
		cache_[type].clear(false);
	}
	return res;
}

bool AssetPropsScript::loadScript(assetDb::AssetType::Enum type, std::string& out)
{
	core::Path<char> path;
	path.appendFmt("assetscripts%c", core::Path<char>::NATIVE_SLASH);
	path.append(assetDb::AssetType::ToString(type));
	path.setExtension(ASSET_PROPS_SCRIPT_EXT);
	path.toLower();

	return loadScript(path, out);
}

bool AssetPropsScript::loadScript(const core::Path<char>& path, std::string& out)
{
	FILE* f = nullptr;
	fopen_s(&f, path.c_str(), "rb");

	if (!f) {
		X_ERROR("AssetScript", "Failed to load file: \"%s\"", path.c_str());
		return false;
	}

	fseek(f, 0, SEEK_END);
	const int32_t len = ftell(f);
	fseek(f, 0, SEEK_SET);

	int32_t read = 0;
	if (len > 0)
	{
		out.resize(len);
		read = static_cast<int32_t>(fread(const_cast<char*>(out.data()), 1, len, f));
	}

	fclose(f);

	if (read != len) {
		out.clear();
		X_ERROR("AssetScript", "Failed to read all of file: \"%s\" requested: % " PRIi32 " got: %" PRIi32, path.c_str(), len, read);
		return false;
	}

	return true;
}

bool AssetPropsScript::processScript(AssetProps& props, Scriptcache& cache)
{
	if (cache.byteCode.isNotEmpty()) {
		return processScript(props, cache.byteCode);
	}

	if (cache.text.empty()) {
		return false;
	}

	return processScript(props, cache.text, &cache.byteCode);
}

bool AssetPropsScript::processScript(AssetProps& props, const std::string& script, ByteCodeStream* pCacheOut)
{
	X_ASSERT_NOT_NULL(pEngine_);

	asIScriptModule* pMod = pEngine_->GetModule("AssetScriptModule", asGM_ALWAYS_CREATE);
	if (!pMod) {
		X_FATAL("AssetScript", "Unrecoverable error while starting a new module.");
		return false;
	}

	int32_t r = pMod->AddScriptSection("AssetScriptModule", script.c_str(), script.size());
	if (r < 0) {
		X_ERROR("AssetScript", "Please correct the errors in script and try again. Err: %" PRIi32, r);
		return false;
	}

	r = pMod->Build();
	if (r < 0) {
		X_ERROR("AssetScript", "Please correct the errors in script and try again. Err: %" PRIi32, r);
		return false;
	}

	if (pCacheOut) {
		pCacheOut->clear();

		r = pMod->SaveByteCode(pCacheOut);
		if (r < 0) {
			X_WARNING("AssetScript", "Failed to save byte code %" PRIi32, r);
			pCacheOut->clear(); // re clear 
		}
	}

	return execScript(props, pMod);
}


bool AssetPropsScript::processScript(AssetProps& props, ByteCodeStream& byteCode)
{
	X_ASSERT_NOT_NULL(pEngine_);

	asIScriptModule* pMod = pEngine_->GetModule("AssetScriptModule", asGM_ALWAYS_CREATE);
	if (!pMod) {
		X_FATAL("AssetScript", "Unrecoverable error while starting a new module.");
		return false;
	}

	int32_t r = pMod->LoadByteCode(&byteCode);
	if (r < 0) {
		X_ERROR("AssetScript", "Please correct the errors in script and try again. Err: %" PRIi32, r);
		return false;
	}

	r = pMod->Build();
	if (r < 0) {
		X_ERROR("AssetScript", "Please correct the errors in script and try again. Err: %" PRIi32, r);
		return false;
	}

	return execScript(props, pMod);
}


bool AssetPropsScript::execScript(AssetProps& props, asIScriptModule* pMod)
{
	asIScriptFunction* pFunc = pMod->GetFunctionByDecl(SCRIPT_ENTRY);
	if (!pFunc)
	{
		X_ERROR("AssetScript", "The script must have the following entry point 'void GenerateUI(asset& ass)'", SCRIPT_ENTRY);
		return false;
	}

	asIScriptContext* pCtx = pEngine_->CreateContext();
	pCtx->Prepare(pFunc);
	pCtx->SetArgObject(0, &props);

	int32_t r = pCtx->Execute();
	if (r != asEXECUTION_FINISHED)
	{
		X_ERROR("AssetScript", "Failed to execute script Err: %" PRIu32, r);
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