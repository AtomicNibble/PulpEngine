#include "stdafx.h"
#include "SettingsCmd.h"
#include "MayaUtil.h"

#include <maya\MArgList.h>
#include <maya\MSyntax.h>

#include "AppDataPath.h"

#include <FileAPI.h>


using namespace core::xml::rapidxml;

X_NAMESPACE_BEGIN(maya)

namespace
{
	void* XmlAllocate(std::size_t size)
	{
		return X_NEW_ARRAY(char, size, g_arena, "xmlPool");
	}

	void XmlFree(void* pointer)
	{
		char* pChar = static_cast<char*>(pointer);
		X_DELETE_ARRAY(pChar, g_arena);
	}


	SettingsCache* gSettingsCache = nullptr;

} // namespace

const char* SettingsCache::SETTINGS_PATH = "PotatoEngine//MayaPlugin//settings.xml";

SettingsCache::SettingsCache() : 
	cacheLoaded_(false), 
	settingsCache_(g_arena)
{
	settingsCache_.reserve(16);
}

SettingsCache::~SettingsCache()
{
}


void SettingsCache::Init(void)
{
	gSettingsCache = X_NEW(SettingsCache, g_arena, "SettingsCache");
}

void SettingsCache::ShutDown(void)
{
	if (gSettingsCache) {
		X_DELETE_AND_NULL(gSettingsCache, g_arena);
	}
}

bool SettingsCache::SetValue(SettingId::Enum id, const core::StackString512& value)
{
	core::StackString<64> name = core::StackString<64>(SetIdToStr(id));

	// load it if not already loaded.
	// before we insert our value.
	if (!cacheLoaded_) {
		ReloadCache();
	}

	SettingsCacheMap::const_iterator it = settingsCache_.find(name);
	if (it != settingsCache_.end()) {
		settingsCache_[name] = value;
	}
	else {
		settingsCache_.insert(std::make_pair(name, value));
	}

	FlushCache();

	return true;
}

bool SettingsCache::GetValue(SettingId::Enum id, core::StackString512& value)
{
	const char* pName = SetIdToStr(id);

	if (!cacheLoaded_) {
		ReloadCache();
	}

	SettingsCacheMap::const_iterator it = settingsCache_.find(core::StackString<64>(pName));
	if (it != settingsCache_.end()) {
		value = it->second;
		return true;
	}

	// empty value
	value.clear();
	return true;
}

const char* SettingsCache::SetIdToStr(SettingId::Enum id)
{
	if (id == SettingId::ANIM_OUT) {
		return "animOut";
	}
	if (id == SettingId::MODEL_OUT) {
		return "modelOut";
	}
	if (id == SettingId::EXPORTMODE) {
		return "exportMode";
	}
	if (id == SettingId::SAVE_IF_CHANGED_MODE) {
		return "saveIfChangedMode";
	}

	X_ASSERT_NOT_IMPLEMENTED();
	return "invalid";
}


bool SettingsCache::ReloadCache(void)
{
	core::Path<char> filePath = GetSettingsPath();

	FILE* pFile;
	fopen_s(&pFile, filePath.c_str(), "r+b");
	if (!pFile) {
		return false;
	}

	fseek(pFile, 0L, SEEK_END);
	size_t fileSize = ftell(pFile);
	fseek(pFile, 0L, SEEK_SET);

	// empty?
	if (fileSize < 1) {
		::fclose(pFile);
		cacheLoaded_ = true;
		return true;
	}

	char* pText = X_NEW_ARRAY(char, fileSize + 1, g_arena, "SettingsXMLBuf");

	size_t bytesRead = ::fread(pText, 1, fileSize, pFile);
	::fclose(pFile);

	if (bytesRead != fileSize) {
		return false;
	}

	// nullterm
	pText[fileSize] = '\0';

	{
		xml_document<> doc;    
		doc.set_allocator(XmlAllocate, XmlFree);
		doc.parse<0>(pText);    

		settingsCache_.clear();

		// parse settings.
		xml_node<>* pSettings = doc.first_node("settings");
		if (pSettings)
		{
			xml_node<>* pSetNode;

			for (pSetNode = pSettings->first_node("entry"); pSetNode;
			pSetNode = pSetNode->next_sibling())
			{
				xml_attribute<>* pAttr = pSetNode->first_attribute("id");
				if (pAttr)
				{
					core::StackString<64> id(pAttr->value(), pAttr->value() + pAttr->value_size());
					core::StackString512 value(pSetNode->value(), 
						pSetNode->value() + pSetNode->value_size());

					settingsCache_.insert(std::make_pair(id, value));
				}
			}
		}
	}

	X_DELETE_ARRAY(pText, g_arena);
	cacheLoaded_ = true;
	return true;
}


bool SettingsCache::FlushCache(void)
{
	xml_document<> doc;
	xml_node<>* decl = doc.allocate_node(node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	doc.append_node(decl);

	xml_node<>* pSettings = doc.allocate_node(node_element, "settings");
	doc.append_node(pSettings);

	SettingsCacheMap::const_iterator it = settingsCache_.begin();
	for (; it != settingsCache_.end(); ++it)
	{
		xml_node<>* pEntry = doc.allocate_node(node_element, "entry", it->second.c_str());
		pEntry->append_attribute(doc.allocate_attribute("id", it->first.c_str()));
		pSettings->append_node(pEntry);
	}

	std::string xml_as_string;
	core::xml::rapidxml::print(std::back_inserter(xml_as_string), doc);

	// save it
	{
		core::Path<char> filePath = GetSettingsPath();

		FILE* pFile;
		fopen_s(&pFile, filePath.c_str(), "w");
		if (pFile)
		{
			::fwrite(xml_as_string.data(), 1, xml_as_string.size(), pFile);
			::fclose(pFile);
			return true;
		}
	}

	return false;
}

core::Path<char> SettingsCache::GetSettingsPath(void)
{
	core::Path<char> path;

	misc::AppDataPath appDataPath;
	if (misc::GetAppDataPath(appDataPath))
	{
		char temp[512] = { '\0' };
		core::strUtil::Convert(appDataPath, temp);

		path.set(temp);
		path.ensureSlash();
	}

	path.append(SETTINGS_PATH);
	path.replaceSeprators();

	// create the path.
	{
		core::Path<char> folder;

		const char* end = path.find(core::Path<char>::NATIVE_SLASH);
		while (end != nullptr)
		{
			folder.set(path.c_str(), end + 1);

			if (!CreateDirectoryA(folder.c_str(), NULL))
			{
				DWORD err = GetLastError();

				if (err != ERROR_ALREADY_EXISTS)
				{
					// do whatever handling you'd like
				}
			}

			end = core::strUtil::Find(++end, core::Path<char>::NATIVE_SLASH);
		}
	}

	return path;
}


// -----------------------------------------------

SettingsCmd::SettingsCmd()
{
}

SettingsCmd::~SettingsCmd()
{
}


MStatus SettingsCmd::doIt(const MArgList & args)
{
	uint idx;
	MStatus status;

	Mode::Enum mode;
	SettingId::Enum setId;

	if (!gSettingsCache) {
		MayaUtil::MayaPrintError("SettingsCache is invalid, Init must be called (source code error)");
		return MS::kFailure;
	}

	// get the mode.
	{
		idx = args.flagIndex("g", "get");
		if (idx != MArgList::kInvalidArgIndex)
		{
			mode = Mode::GET;
		}
		else
		{
			idx = args.flagIndex("s", "set");
			if (idx == MArgList::kInvalidArgIndex)
			{
				MayaUtil::MayaPrintError("unknown mode valid modes: set, get");
				return MS::kFailure;
			}

			mode = Mode::SET;
		}
	}

	// get the path id.
	{

		idx = args.flagIndex("pi", "set_id");
		if (idx == MArgList::kInvalidArgIndex) {
			MayaUtil::MayaPrintError("set_id flag missing");
			return MS::kFailure;
		}

		MString setIdStr;
		if (!(status = args.get(++idx, setIdStr))) {
			MayaUtil::MayaPrintError("failed to get set_id flag value: %s", 
				status.errorString().asChar());
			return MS::kFailure;
		}

		// is it a valid path id?
		if (core::strUtil::IsEqualCaseInsen(setIdStr.asChar(), "animOut")) {
			setId = SettingId::ANIM_OUT;
		}
		else if (core::strUtil::IsEqualCaseInsen(setIdStr.asChar(), "modelOut")) {
			setId = SettingId::MODEL_OUT;
		}
		else if (core::strUtil::IsEqualCaseInsen(setIdStr.asChar(), "exportMode")) {
			setId = SettingId::EXPORTMODE;
		}
		else if (core::strUtil::IsEqualCaseInsen(setIdStr.asChar(), "saveIfChangedMode")) {
			setId = SettingId::SAVE_IF_CHANGED_MODE;
		}
		else
		{
			MayaUtil::MayaPrintError("unknown set_id: '%s' valid id's: animOut, modelOut, exportMode, saveIfChangedMode", 
				setIdStr.asChar());
			return MS::kFailure;
		}
	}


	// if set mode we also need a value.
	if (mode == Mode::SET)
	{
		idx = args.flagIndex("v", "value");
		if (idx == MArgList::kInvalidArgIndex) {
			MayaUtil::MayaPrintError("value flag missing");
			return MS::kFailure;
		}

		MString valueStr;
		if (!(status = args.get(++idx, valueStr))) {
			MayaUtil::MayaPrintError("failed to get value flag value: %s", status.errorString().asChar());
			return MS::kFailure;
		}

		if (setId == SettingId::ANIM_OUT || setId == SettingId::MODEL_OUT)
		{
			core::Path<char> newValue;
			newValue.append(valueStr.asChar());
			newValue.replaceSeprators();
			newValue.ensureSlash();

			if (!gSettingsCache->SetValue(setId, 
				core::StackString512(newValue.begin(), newValue.end()))) {
				return MS::kFailure;
			}
		}
		else
		{
			core::StackString512 newValue(valueStr.asChar());

			if (!gSettingsCache->SetValue(setId,newValue)) {
				return MS::kFailure;
			}
		}
	}
	else
	{
		core::StackString512 value;

		if (!gSettingsCache->GetValue(setId, value)) {
			return MS::kFailure;
		}

		// escale the slashes.
		value.replaceAll("\\", "/");

		MString mayaValue(value.c_str());
		setResult(mayaValue);
	}

	return MS::kSuccess;
}


void* SettingsCmd::creator(void)
{
	return new SettingsCmd;
}

MSyntax SettingsCmd::newSyntax(void)
{
	MSyntax syn;

	syn.addFlag("-g", "-get", MSyntax::kString);
	syn.addFlag("-s", "-set", MSyntax::kString);
	syn.addFlag("-v", "-value", MSyntax::kString);
	syn.addFlag("-pi", "-set_id", MSyntax::kString);

	return syn;
}

X_NAMESPACE_END