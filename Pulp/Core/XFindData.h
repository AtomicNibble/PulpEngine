#pragma once

#ifndef _X_XFIND_DATA_H_
#define _X_XFIND_DATA_H_

X_NAMESPACE_BEGIN(core)


struct XFindData
{
	XFindData(const char* path, xFileSys* pFileSys) :
	path_(path),
	handle_(-1),
	pFileSys_(pFileSys),
	current_(pFileSys->searchPaths_)
	{
		folder_ = path_;
		folder_.removeFileName();
		folder_.ensureSlash();
	}

	~XFindData() {}

	bool findnext(_finddatai64_t* fi) {
		if (!current_)
			return returnFalse(fi);

		if (current_->dir)
			return searchDir(current_->dir, fi);
		return searchPak(fi);
	}

	bool getOSPath(core::Path<char>& path, _finddatai64_t* fi)
	{
		X_ASSERT_NOT_NULL(fi);

		if (current_->dir) {
			path /= current_->dir->path / fi->name;
			return true;
		}

		return false;
	}

private:

	bool searchDir(directory_s* dir, _finddatai64_t* fi) {

		if (handle_ == -1) {
			// new search dir.
			wchar_t wPath[MAX_PATH];

			Path<char> temp;

			core::zero_object(fdw);
			core::strUtil::Convert(pFileSys_->createOSPath(dir, path_.c_str(), temp), wPath, MAX_PATH);

			// we have:
			// C:\\Users\\Tom\\Documents\\Visual Studio 2013\\Projects\\WinEngine\\code\\game_folder\\
			// core_assets/white.dds

			// then gives us:
			// white.dds
			// we need to make it back into relative path.

			handle_ = _wfindfirst64(wPath, &fdw);

			if (handle_ == -1) // if nothing found in this dir, move on.
				return returnFindhNext(fi);
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

	inline void updateFindInfo(_finddatai64_t* fi) {
		char temp[MAX_PATH];
		fi->attrib = fdw.attrib;
		fi->size = fdw.size;
		fi->time_access = fdw.time_access;
		fi->time_create = fdw.time_create;
		fi->time_write = fdw.time_write;
		strcpy_s(fi->name, folder_.c_str());
		strcat_s(fi->name, core::strUtil::Convert(fdw.name, temp));
	}

	bool searchPak(_finddatai64_t* fi) {
		return returnFalse(fi);
	}

	inline static bool returnFalse(_finddatai64_t* fi) {
		core::zero_this(fi);
		return false;
	}

	inline bool returnFindhNext(_finddatai64_t* findinfo) {
		current_ = current_->next_;
		return findnext(findinfo);
	}

private:
	Path<char>		path_;
	Path<char>		folder_;

	intptr_t		handle_;
	_wfinddata64_t	fdw;

	search_s*		current_;
	xFileSys*		pFileSys_;
};


X_NAMESPACE_END

#endif // !_X_XFIND_DATA_H_
