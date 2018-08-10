#pragma once

#ifndef X_DIRECTORY_WATCHER_I_H_
#define X_DIRECTORY_WATCHER_I_H_

#include <String\Path.h>

X_NAMESPACE_BEGIN(core)

struct IDirectoryWatcherListener;

struct IDirectoryWatcher
{
    X_DECLARE_ENUM(Action)(
        ADDED,    ///< A file or directory has been added.
        REMOVED,  ///< A file or directory has been removed.
        MODIFIED, ///< A file or directory has been modified.
        RENAMED   ///< A file or directory has been renamed.
    );

    virtual ~IDirectoryWatcher() = default;

    virtual void addDirectory(const char* directory) X_ABSTRACT;
    virtual void addDirectory(const wchar_t* directory) X_ABSTRACT;

    virtual void registerListener(IDirectoryWatcherListener* pListener) X_ABSTRACT;
    virtual void unregisterListener(IDirectoryWatcherListener* pListener) X_ABSTRACT;
};

struct IDirectoryWatcherListener
{
    virtual ~IDirectoryWatcherListener() = default;

    // returns true if it action was eaten.
    virtual bool OnFileChange(IDirectoryWatcher::Action::Enum action,
        const char* pName, const char* pOldName, bool isDirectory) X_ABSTRACT;
};


struct IXHotReload
{
    virtual ~IXHotReload() = default;

    virtual void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& path) X_ABSTRACT;
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
