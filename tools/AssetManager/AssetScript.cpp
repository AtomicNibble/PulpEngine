#include "AssetScript.h"
#include "AssetScriptTypes.h"
#include "AssetPropertyEditor.h"

#include "assetscript\scriptstdstring.h"
#include "assetscript\scriptbuilder.h"

#include "AssetEntryManager.h"

#include <time\StopWatch.h>
#include <String\HumanDuration.h>

#include <../../tools/MaterialLib/MatLib.h>


X_NAMESPACE_BEGIN(assman)

namespace
{


} // namepace 

AssetPropsScriptManager::ByteCodeStream::ByteCodeStream() :
	readPos_(0)
{

}

bool AssetPropsScriptManager::ByteCodeStream::isNotEmpty(void) const
{
	return !data_.empty();
}

void AssetPropsScriptManager::ByteCodeStream::clear(void)
{
	readPos_ = 0;
	data_.clear();
}

void AssetPropsScriptManager::ByteCodeStream::resetRead(void)
{
	readPos_ = 0;
}


void AssetPropsScriptManager::ByteCodeStream::Write(const void *ptr, asUINT size)
{
	if (size == 0) {
		return;
	}

	const size_t writePos = data_.size();
	data_.resize(data_.size() + size);
	std::memcpy(&data_[writePos], ptr, size);
}

void AssetPropsScriptManager::ByteCodeStream::Read(void *ptr, asUINT size)
{
	if (size == 0) {
		return;
	}

	if ((size + readPos_) > data_.size()) {
		size = static_cast<uint32_t>( data_.size() - readPos_);
	}

	std::memcpy(ptr, &data_[readPos_], size);
	readPos_ += size;
}

// ----------------------------------------------------------

void AssetPropsScriptManager::Scriptcache::clear(bool byteCodeOnly)
{
	if (!byteCodeOnly) {
		text.clear();
	}
	byteCode.clear();
}

// ----------------------------------------------------------

const char* AssetPropsScriptManager::ASSET_PROPS_SCRIPT_DIR = "assetscripts";
const char* AssetPropsScriptManager::ASSET_PROPS_SCRIPT_EXT = "aps";
const char* AssetPropsScriptManager::SCRIPT_ENTRY = "void GenerateUI(asset& AssetProps)";


AssetPropsScriptManager::AssetPropsScriptManager() :
	pEngine_(nullptr),
	pWatcher_(nullptr),
	pTechDefs_(nullptr)
{
	// cache_.fill(nullptr);
}

AssetPropsScriptManager::~AssetPropsScriptManager()
{
	shutdown();

	if (pWatcher_) {
		delete pWatcher_;
	}
	if (pTechDefs_) {
		delete pTechDefs_;
	}
}

bool AssetPropsScriptManager::init(bool enableHotReload)
{
	if (enableHotReload && !pWatcher_) {
		pWatcher_ = new QFileSystemWatcher(this);

		connect(pWatcher_, SIGNAL(fileChanged(const QString &)),
			this, SLOT(fileChanged(const QString &)));
	}

	if (!pTechDefs_) {
		pTechDefs_ = new engine::techset::TechSetDefs(g_arena);
	}


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
	r = pEngine_->RegisterObjectType("assetProp", sizeof(AssetScriptProperty), asOBJ_REF);  BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("assetProp", asBEHAVE_FACTORY, "assetProp@ f()", asFUNCTION(AssetScriptProperty::factory), asCALL_CDECL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("assetProp", asBEHAVE_FACTORY, "assetProp@ f(const assetProp& in)", asFUNCTION(AssetScriptProperty::copyFactory), asCALL_CDECL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("assetProp", asBEHAVE_ADDREF, "void f()", asMETHOD(AssetScriptProperty, addRef), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("assetProp", asBEHAVE_RELEASE, "void f()", asMETHOD(AssetScriptProperty, release), asCALL_THISCALL); BUG_CHECK(r >= 0);
	
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp &opAssign(const assetProp &in)", asMETHODPR(AssetScriptProperty, operator=, (const AssetScriptProperty&), AssetScriptProperty&), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetTitle(const string &in)", asMETHOD(AssetScriptProperty, SetTitle), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetToolTip(const string &in)", asMETHOD(AssetScriptProperty, SetToolTip), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetLabels(const string &in, const string &in, const string &in, const string &in)", asMETHOD(AssetScriptProperty, SetLabels), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetIcon(const string &in)", asMETHOD(AssetScriptProperty, SetIcon), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetFontColor(float r, float g, float b)", asMETHOD(AssetScriptProperty, SetFontColor), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetBold(bool)", asMETHOD(AssetScriptProperty, SetBold), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetStep(double step)", asMETHOD(AssetScriptProperty, SetStep), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetEnabled(bool)", asMETHOD(AssetScriptProperty, SetEnabled), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetVisible(bool)", asMETHOD(AssetScriptProperty, SetVisible), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ ShowJumpToAsset(bool)", asMETHOD(AssetScriptProperty, ShowJumpToAsset), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ UpdateOnChange(bool)", asMETHOD(AssetScriptProperty, UpdateOnChange), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetValue(const string &in)", asMETHOD(AssetScriptProperty, SetValue), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetDefaultValue(const string &in)", asMETHOD(AssetScriptProperty, SetDefaultValue), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetBool(bool val)", asMETHOD(AssetScriptProperty, SetBool), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetInt(int val)", asMETHOD(AssetScriptProperty, SetInt), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetFloat(float val)", asMETHOD(AssetScriptProperty, SetFloat), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "assetProp@ SetDouble(double val)", asMETHOD(AssetScriptProperty, SetDouble), asCALL_THISCALL); BUG_CHECK(r >= 0);

	r = pEngine_->RegisterObjectMethod("assetProp", "string GetTitle()", asMETHOD(AssetScriptProperty, GetTitle), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "string GetToolTip()", asMETHOD(AssetScriptProperty, GetToolTip), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "string GetValue()", asMETHOD(AssetScriptProperty, GetValue), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "double GetFloat()", asMETHOD(AssetScriptProperty, GetValueFloat), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "int GetInt()", asMETHOD(AssetScriptProperty, GetValueInt), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("assetProp", "bool GetBool()", asMETHOD(AssetScriptProperty, GetValueBool), asCALL_THISCALL); BUG_CHECK(r >= 0);


	// asset
	r = pEngine_->RegisterObjectType("asset", sizeof(AssetScriptProps), asOBJ_REF); BUG_CHECK(r >= 0);
//	r = pEngine_->RegisterObjectBehaviour("asset", asBEHAVE_FACTORY, "asset@ f()", asFUNCTION(AssetScriptProps::factory), asCALL_CDECL); BUG_CHECK(r >= 0);
//	r = pEngine_->RegisterObjectBehaviour("asset", asBEHAVE_FACTORY, "asset@ f(const asset& in)", asFUNCTION(AssetScriptProps::copyFactory), asCALL_CDECL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("asset", asBEHAVE_ADDREF, "void f()", asMETHOD(AssetScriptProps, addRef), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectBehaviour("asset", asBEHAVE_RELEASE, "void f()", asMETHOD(AssetScriptProps, release), asCALL_THISCALL); BUG_CHECK(r >= 0);
	
//	r = pEngine_->RegisterObjectMethod("asset", "asset &opAssign(const asset &in)", asMETHODPR(AssetScriptProps, operator=, (const AssetScriptProps&), AssetScriptProps&), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddFont(const string& in, const string& in)", asMETHOD(AssetScriptProps, AddFont), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddTexture(const string& in, const string& in)", asMETHOD(AssetScriptProps, AddTexture), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddModel(const string& in, const string& in)", asMETHOD(AssetScriptProps, AddModel), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddAnim(const string& in, const string& in)", asMETHOD(AssetScriptProps, AddAnim), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddAssetRef(const string& in, const string& in)", asMETHOD(AssetScriptProps, AddAssetRef), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddCombo(const string& in, const string, bool editiable = false)", asMETHOD(AssetScriptProps, AddCombo), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddCheckBox(const string& in, bool)", asMETHOD(AssetScriptProps, AddCheckBox), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddInt(const string& in, uint, uint, uint)", asMETHOD(AssetScriptProps, AddInt), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddFloat(const string& in, double val, double min, double max)", asMETHOD(AssetScriptProps, AddFloat), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddColor(const string& in, double r, double g, double b, double a)", asMETHOD(AssetScriptProps, AddColor), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddVec2(const string& in, const string& in, double x, double y,  double min, double max)", asMETHOD(AssetScriptProps, AddVec2), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddVec3(const string& in, const string& in, const string& in, double x, double y, double z, double min, double max)", asMETHOD(AssetScriptProps, AddVec3), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddVec4(const string& in, const string& in, const string& in, const string& in, double x, double y, double z, double w, double min, double max)", asMETHOD(AssetScriptProps, AddVec4), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddText(const string& in, const string& in)", asMETHOD(AssetScriptProps, AddText), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddPath(const string& in, const string& in)", asMETHOD(AssetScriptProps, AddPath), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddString(const string& in, const string& in)", asMETHOD(AssetScriptProps, AddString), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ AddLabel(const string& in, const string& in)", asMETHOD(AssetScriptProps, AddLabel), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "void BeginGroup(const string& in)", asMETHOD(AssetScriptProps, BeginGroup), asCALL_THISCALL); BUG_CHECK(r >= 0);

	r = pEngine_->RegisterObjectMethod("asset", "assetProp@ getItem(const string& in)", asMETHOD(AssetScriptProps, getItem), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "string getPropValue(const string& in)", asMETHOD(AssetScriptProps, getPropValue), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "double getPropValueFloat(const string& in)", asMETHOD(AssetScriptProps, getPropValueFloat), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "int getPropValueInt(const string& in)", asMETHOD(AssetScriptProps, getPropValueInt), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "bool getPropValueBool(const string& in)", asMETHOD(AssetScriptProps, getPropValueBool), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "void showProp(const string& in)", asMETHOD(AssetScriptProps, showProp), asCALL_THISCALL); BUG_CHECK(r >= 0);

	r = pEngine_->RegisterObjectMethod("asset", "string getMaterialCats()", asMETHOD(AssetScriptProps, getMaterialCats), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "string getMaterialTypes(const string& in)", asMETHOD(AssetScriptProps, getMaterialTypes), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "bool isMaterialType(const string& in, const string& in)", asMETHOD(AssetScriptProps, isMaterialType), asCALL_THISCALL); BUG_CHECK(r >= 0);
	r = pEngine_->RegisterObjectMethod("asset", "void addMaterialTypeProps(const string& in, const string& in)", asMETHOD(AssetScriptProps, addMaterialTypeProps), asCALL_THISCALL); BUG_CHECK(r >= 0);


	return true;
}

void AssetPropsScriptManager::shutdown(void)
{
	if (pEngine_) {
		pEngine_->ShutDownAndRelease();
		pEngine_ = nullptr;
	}

	clearCache();
}


bool AssetPropsScriptManager::runScriptForProps(AssetProperties& props, assetDb::AssetType::Enum type)
{
	if (!ensureSourceCache(type)) {
		X_ERROR("AssetScript", "Failed to get assetProps for asset type: \"%s\"", assetDb::AssetType::ToString(type));
		return false;
	}

	props.setAssetType(type);

	return processScript(props, cache_[type]);
}


void AssetPropsScriptManager::clearCache(bool byteCodeOnly)
{
	for(auto& entry : cache_) {
		entry.clear(byteCodeOnly);
	}
}

void AssetPropsScriptManager::clearCache(assetDb::AssetType::Enum type, bool byteCodeOnly)
{
	cache_[type].clear(byteCodeOnly);
}

bool AssetPropsScriptManager::sourceCacheValid(assetDb::AssetType::Enum type) const
{
	return !cache_[type].text.empty();
}

bool AssetPropsScriptManager::ensureSourceCache(assetDb::AssetType::Enum type, bool reload)
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

bool AssetPropsScriptManager::loadScript(assetDb::AssetType::Enum type, std::string& Out)
{
	core::Path<char> path;
	path.appendFmt("%s%c", ASSET_PROPS_SCRIPT_DIR, core::Path<char>::NATIVE_SLASH);
	path.append(assetDb::AssetType::ToString(type));
	path.setExtension(ASSET_PROPS_SCRIPT_EXT);
	path.toLower();

	bool res = loadScript(path, Out);

	if(res && pWatcher_) {
		pWatcher_->addPath(path.c_str());
	}

	return res;
}

bool AssetPropsScriptManager::loadScript(const core::Path<char>& path, std::string& out)
{
	FILE* f = nullptr;
	int32_t retryCount = 0;

retry:
	auto err = fopen_s(&f, path.c_str(), "rb");

	if (!f) {

		// text editor not fully closed handle.
		// give it upto 100ms to.
		if (err == EACCES && retryCount < 10) {
			retryCount++;
			core::Thread::Sleep(10);
			goto retry;
		}

		X_ERROR("AssetScript", "Failed to load file(%" PRIi32 "): \"%s\"", err, path.c_str());
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

bool AssetPropsScriptManager::processScript(AssetProperties& props, Scriptcache& cache)
{
	if (cache.byteCode.isNotEmpty()) {
		return processScript(props, cache.byteCode);
	}

	if (cache.text.empty()) {
		return false;
	}

	return processScript(props, cache.text, &cache.byteCode);
}

bool AssetPropsScriptManager::processScript(AssetProperties& props, const std::string& script, ByteCodeStream* pCacheOut)
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


bool AssetPropsScriptManager::processScript(AssetProperties& props, ByteCodeStream& byteCode)
{
	X_ASSERT_NOT_NULL(pEngine_);

	asIScriptModule* pMod = pEngine_->GetModule("AssetScriptModule", asGM_ALWAYS_CREATE);
	if (!pMod) {
		X_FATAL("AssetScript", "Unrecoverable error while starting a new module.");
		return false;
	}

	byteCode.resetRead();

	int32_t r = pMod->LoadByteCode(&byteCode);
	if (r < 0) {
		X_ERROR("AssetScript", "Please correct the errors in script and try again. Err: %" PRIi32, r);
		return false;
	}

// don't need to rebuild byte code.
//	r = pMod->Build();
//	if (r < 0) {
//		X_ERROR("AssetScript", "Please correct the errors in script and try again. Err: %" PRIi32, r);
//		return false;
//	}

	return execScript(props, pMod);
}


bool AssetPropsScriptManager::execScript(AssetProperties& props, asIScriptModule* pMod)
{
	asIScriptFunction* pFunc = pMod->GetFunctionByDecl(SCRIPT_ENTRY);
	if (!pFunc)
	{
		X_ERROR("AssetScript", "The script must have the following entry point 'void GenerateUI(asset& ass)'", SCRIPT_ENTRY);
		return false;
	}

	core::StopWatch timer;

	AssetScriptProps scriptProps(props, *pTechDefs_);

	asIScriptContext* pCtx = pEngine_->CreateContext();
	pCtx->Prepare(pFunc);
	pCtx->SetArgObject(0, &scriptProps);

	int32_t r = pCtx->Execute();
 	if (r != asEXECUTION_FINISHED)
	{
		X_ERROR("AssetScript", "Failed to execute script Err: %" PRIu32, r);
		if (r == asEXECUTION_EXCEPTION) {
			printExceptionInfo(pCtx);
		}

		return false;
	}

	core::HumanDuration::Str timeStr;
	X_LOG1("AssetScript", "Script exec took: ^6%s", core::HumanDuration::toString(timeStr, timer.GetMilliSeconds()));
	return true;
}

void AssetPropsScriptManager::messageCallback(const asSMessageInfo* msg, void* param)
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

void AssetPropsScriptManager::printExceptionInfo(asIScriptContext * pCtx)
{
	const asIScriptFunction *function = pCtx->GetExceptionFunction();

	X_ERROR("AssetScript", "An exception occurred. Please correct the code and try again.");
	X_ERROR("AssetScript", "desc: %s", pCtx->GetExceptionString());
	X_ERROR("AssetScript", "func: %s", function->GetDeclaration());
	X_ERROR("AssetScript", "modl: %s", function->GetModuleName());
	X_ERROR("AssetScript", "sect: %s", function->GetScriptSectionName());
	X_ERROR("AssetScript", "line: %d", pCtx->GetExceptionLineNumber());
}


void AssetPropsScriptManager::fileChanged(const QString& path)
{
	X_UNUSED(path);

	// we only watch files we have loaded
	// so it's going to be a aps.

	// just map name to type.
	QFile file(path);
	QFileInfo fileInfo(file);
	QString fileName = fileInfo.fileName();
	fileName.toLower();

	X_LOG1("AssetScript", "Aps script edited: %ls", fileName.data());

	for (uint32_t i=0; i<assetDb::AssetType::ENUM_COUNT; i++)
	{
		const auto type = static_cast<assetDb::AssetType::Enum>(i);
		QString typeFileName(assetDb::AssetType::ToString(type));
		typeFileName = typeFileName.toLower();
		typeFileName.append('.');
		typeFileName.append(ASSET_PROPS_SCRIPT_EXT);
		
		if (fileName == typeFileName)
		{
			X_LOG1("AssetScript", "Cache cleared for aps for type: %s ", assetDb::AssetType::ToString(i));

			cache_[i].clear(false);

			AssetEntryManager::reloadUIforType(type);
			break;
		}
	}
}



X_NAMESPACE_END