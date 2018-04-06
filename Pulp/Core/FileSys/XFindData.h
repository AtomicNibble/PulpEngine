#pragma once

#ifndef _X_XFIND_DATA_H_
#define _X_XFIND_DATA_H_

X_NAMESPACE_BEGIN(core)

class xFileSys;
struct Directory;
struct Pak;
struct Search;

struct XFindData
{
    XFindData(const wchar_t* path, xFileSys* pFileSys);
    XFindData(const char* path, xFileSys* pFileSys);
    ~XFindData();

    bool findnext(_wfinddatai64_t* fi);
    bool getOSPath(core::Path<wchar_t>& path, _wfinddatai64_t* fi);

private:
    bool searchDir(Directory* dir, _wfinddatai64_t* fi);

    inline void updateFindInfo(_wfinddatai64_t* fi);
    inline bool searchPak(_wfinddatai64_t* fi);
    inline static bool returnFalse(_wfinddatai64_t* fi);
    inline bool returnFindhNext(_wfinddatai64_t* findinfo);

private:
    Path<wchar_t> path_;
    Path<wchar_t> folder_;

    intptr_t handle_;
    _wfinddata64_t fdw;

    Search* current_;
    xFileSys* pFileSys_;
};

X_NAMESPACE_END

#endif // !_X_XFIND_DATA_H_
