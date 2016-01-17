#include "stdafx.h"
#include "PathCmds.h"
#include "MayaUtil.h"

#include <maya\MArgList.h>
#include <maya\MSyntax.h>

#include <String\Path.h>
#include <String\Xml.h>

#include "AppDataPath.h"

#include <FileAPI.h>

using namespace core::xml::rapidxml;

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
} // namespace

const char* PathCmd::SETTINGS_PATH = "PotatoEngine//MayaPlugin//settings.xml";

PathCmd::PathCmd() : cacheLoaded_(false), PathCache_(g_arena)
{
	PathCache_.reserve(4);
}

PathCmd::~PathCmd()
{
}

MStatus PathCmd::doIt(const MArgList & args)
{
	uint idx;
	MStatus status;

	Mode::Enum mode;
	PathId::Enum pathId;

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
				MayaUtil::MayaPrintError("unkown mode valid modes: set, get");
				return MS::kFailure;
			}

			mode = Mode::SET;
		}
	}

	// get the path id.
	{

		idx = args.flagIndex("pi", "path_id");
		if (idx == MArgList::kInvalidArgIndex) {
			MayaUtil::MayaPrintError("path_id flag missing");
			return MS::kFailure;
		}

		MString pathIdStr;
		if (!(status = args.get(++idx, pathIdStr))) {
			MayaUtil::MayaPrintError("failed to get path_id flag value: %s", status.errorString().asChar());
			return MS::kFailure;
		}

		// is it a valid path id?
		if (core::strUtil::IsEqualCaseInsen(pathIdStr.asChar(), "animOut")) {
			pathId = PathId::ANIM_OUT;
		}
		else if (core::strUtil::IsEqualCaseInsen(pathIdStr.asChar(), "modelOut")) {
			pathId = PathId::MODEL_OUT;
		}
		else
		{
			MayaUtil::MayaPrintError("unkown path_id: '%s' valid id's: animOut, modelOut", pathIdStr.asChar());
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

		core::Path<char> newValue;
		newValue.append(valueStr.asChar());
		newValue.replaceSeprators();
		newValue.ensureSlash();

		if (!SetValue(pathId, newValue)) {
			return MS::kFailure;
		}
	}
	else
	{
		core::Path<char> value;

		if (!GetValue(pathId, value)) {
			return MS::kFailure;
		}

		MString mayaValue(value.c_str());
		setResult(mayaValue);
	}

	return MS::kSuccess;
}


void* PathCmd::creator(void)
{
	return new PathCmd;
}

MSyntax PathCmd::newSyntax(void)
{
	MSyntax syn;

	syn.addFlag("-g", "-get", MSyntax::kString);
	syn.addFlag("-s", "-set", MSyntax::kString);
	syn.addFlag("-v", "-value", MSyntax::kString);
	syn.addFlag("-pi", "-path_id", MSyntax::kString);

	return syn;
}

bool PathCmd::SetValue(PathId::Enum id, core::Path<char> value)
{
	core::StackString<64> name = core::StackString<64>(PathIdToStr(id));

	value.replaceSeprators();

	// load it if not already loaded.
	// before we insert our value.
	if (!cacheLoaded_) {
		ReloadCache();
	}

	PathCacheMap::const_iterator it = PathCache_.find(name);
	if (it != PathCache_.end()) {
		PathCache_[name] = value;
	}
	else {
		PathCache_.insert(std::make_pair(name, value));
	}

	FlushCache();

	return true;
}

bool PathCmd::GetValue(PathId::Enum id, core::Path<char>& value)
{
	const char* pName = PathIdToStr(id);

	if (!cacheLoaded_) {
		ReloadCache();
	}

	PathCacheMap::const_iterator it = PathCache_.find(core::StackString<64>(pName));
	if (it != PathCache_.end()) {
		value = it->second;
		return true;
	}

	// empty value
	value.clear();
	return true;
}

const char* PathCmd::PathIdToStr(PathId::Enum id)
{
	if (id == PathId::ANIM_OUT)
		return "AnimOut";
	if (id == PathId::MODEL_OUT)
		return "ModelOut";

	X_ASSERT_NOT_IMPLEMENTED();
	return "invalid";
}


bool PathCmd::ReloadCache(void)
{
	core::Path<char> filePath = GetSettingsPath();

	FILE* pFile;
	fopen_s(&pFile, filePath.c_str(), "r+b");
	if (!pFile)
	{
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

		PathCache_.clear();

		// parse paths.
		xml_node<>* paths = doc.first_node("paths");
		if (paths)
		{
			xml_node<>* pathNode;

			for (pathNode = paths->first_node("path"); pathNode;
			pathNode = pathNode->next_sibling())
			{
				xml_attribute<>* attr = pathNode->first_attribute("id");
				if (attr)
				{
					core::StackString<64> id(attr->value(), attr->value() + attr->value_size());
					core::Path<char> path(pathNode->value(), pathNode->value() + pathNode->value_size());

					PathCache_.insert(std::make_pair(id, path));
				}
			}
		}
	}

	X_DELETE_ARRAY(pText, g_arena);
	cacheLoaded_ = true;
	return true;
}


bool PathCmd::FlushCache(void)
{
	xml_document<> doc;
	xml_node<>* decl = doc.allocate_node(node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	doc.append_node(decl);

	xml_node<>* paths = doc.allocate_node(node_element, "paths");
	doc.append_node(paths);

	PathCacheMap::const_iterator it = PathCache_.begin();
	for (; it != PathCache_.end(); ++it)
	{
		xml_node<>* path = doc.allocate_node(node_element, "path", it->second.c_str());
		path->append_attribute(doc.allocate_attribute("id", it->first.c_str()));
		paths->append_node(path);
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

core::Path<char> PathCmd::GetSettingsPath(void)
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