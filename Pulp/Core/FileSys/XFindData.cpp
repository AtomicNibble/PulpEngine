#include "stdafx.h"
#include "XFindData.h"

#include <io.h>
#include "xFileSys.h"

X_NAMESPACE_BEGIN(core)


XFindData::XFindData(const wchar_t* path, xFileSys* pFileSys) :
	path_(path),
	handle_(-1),
	current_(pFileSys->searchPaths_),
	pFileSys_(pFileSys)
{
	folder_ = path_;
	folder_.removeFileName();
	folder_.replaceSeprators();
	folder_.ensureSlash();
}

XFindData::XFindData(const char* path, xFileSys* pFileSys) :
	handle_(-1),
	current_(pFileSys->searchPaths_),
	pFileSys_(pFileSys)
{
	wchar_t pathW[512];

	path_ = strUtil::Convert(path, pathW, sizeof(pathW));

	folder_ = path_;
	folder_.removeFileName();
	folder_.replaceSeprators();
	folder_.ensureSlash();
}


XFindData::~XFindData() 
{

}

bool XFindData::findnext(_wfinddatai64_t* fi) 
{
	if (!current_) {
		return returnFalse(fi);
	}

	if (current_->dir) {
		return searchDir(current_->dir, fi);
	}

	return searchPak(fi);
}

bool XFindData::getOSPath(core::Path<wchar_t>& path, _wfinddatai64_t* fi)
{
	X_ASSERT_NOT_NULL(fi);

	if (current_->dir) {
		path /= current_->dir->path / fi->name;
		return true;
	}

	return false;
}

bool XFindData::searchDir(directory_s* dir, _wfinddatai64_t* fi)
{
	if (handle_ == -1) {
		// new search dir.
		Path<wchar_t> temp;

		core::zero_object(fdw);
		pFileSys_->createOSPath(dir, path_.c_str(), temp);

		// we have:
		// C:\\Users\\Tom\\Documents\\Visual Studio 2013\\Projects\\WinEngine\\code\\game_folder\\
		// core_assets/white.dds

		// then gives us:
		// white.dds
		// we need to make it back into relative path.

		handle_ = _wfindfirst64(temp.c_str(), &fdw);

		if (handle_ == -1) { // if nothing found in this dir, move on.
			return returnFindhNext(fi);
		}

		// we have found somthing.
		updateFindInfo(fi);
		return true;
	}

	// find next.
	if (_wfindnext64(handle_, &fdw) == 0)
	{
		updateFindInfo(fi);
		return true;
	}

	// no more files or a error.
	// close the handle.
	_findclose(handle_);
	handle_ = -1;

	// search next folder.
	return returnFindhNext(fi);
}

inline void XFindData::updateFindInfo(_wfinddatai64_t* fi)
{
	fi->attrib = fdw.attrib;
	fi->size = fdw.size;
	fi->time_access = fdw.time_access;
	fi->time_create = fdw.time_create;
	fi->time_write = fdw.time_write;
	wcscpy_s(fi->name, folder_.c_str());
	wcscat_s(fi->name, fdw.name);
}

inline bool XFindData::searchPak(_wfinddatai64_t* fi)
{
	return returnFalse(fi);
}

inline bool XFindData::returnFalse(_wfinddatai64_t* fi)
{
	core::zero_this(fi);
	return false;
}

inline bool XFindData::returnFindhNext(_wfinddatai64_t* findinfo)
{
	current_ = current_->pNext;
	return findnext(findinfo);
}


X_NAMESPACE_END
