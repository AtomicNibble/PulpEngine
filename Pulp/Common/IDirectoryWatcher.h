#pragma once

#ifndef X_DIRECTORY_WATCHER_I_H_
#define X_DIRECTORY_WATCHER_I_H_

#include <String\Path.h>

X_NAMESPACE_BEGIN(core)

class XDirectoryWatcherListener;

struct IXDirectoryWatcher
{
    virtual void addDirectory(const char* directory) X_ABSTRACT;
    virtual void addDirectory(const wchar_t* directory) X_ABSTRACT;

    virtual void registerListener(XDirectoryWatcherListener* pListener) X_ABSTRACT;
    virtual void unregisterListener(XDirectoryWatcherListener* pListener) X_ABSTRACT;

protected:
    virtual ~IXDirectoryWatcher()
    {
    }
};

struct IXHotReload
{
    virtual void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& path) X_ABSTRACT;

protected:
    virtual ~IXHotReload()
    {
    }
};

struct IXHotReloadManager
{
    // needs to be done before startup finished.
    // after that it's not thread safe.
    virtual bool addfileType(IXHotReload* pHotReload, const char* extension) X_ABSTRACT;
    virtual void unregisterListener(IXHotReload* pHotReload) X_ABSTRACT;

protected:
    virtual ~IXHotReloadManager()
    {
    }
};

X_NAMESPACE_END

#endif // !X_DIRECTORY_WATCHER_I_H_
