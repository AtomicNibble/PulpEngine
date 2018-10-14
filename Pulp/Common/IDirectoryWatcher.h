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

    virtual void addDirectory(const core::Path<char>& directory) X_ABSTRACT;
    virtual void addDirectory(const core::Path<wchar_t>& directory) X_ABSTRACT;

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

X_NAMESPACE_END

#endif // !X_DIRECTORY_WATCHER_I_H_
