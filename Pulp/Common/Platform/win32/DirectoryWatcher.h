#pragma once

#ifndef X_DIRECTORY_WATCHER_H_
#define X_DIRECTORY_WATCHER_H_

#include "String\Path.h"

#include "Containers\Fifo.h"
#include "Containers\Array.h"

#include <set>

#include <IDirectoryWatcher.h>

X_NAMESPACE_BEGIN(core)

class XDirectoryWatcher : public IXDirectoryWatcher
{
public:
    /// Denotes the action that has occurred.
    struct Action
    {
        enum Enum
        {
            ADDED,    ///< A file or directory has been added.
            REMOVED,  ///< A file or directory has been removed.
            MODIFIED, ///< A file or directory has been modified.
            RENAMED   ///< A file or directory has been renamed.
        };
    };

    explicit XDirectoryWatcher(core::MemoryArenaBase* arena);
    ~XDirectoryWatcher(void);

    void Init(void);
    void ShutDown(void);

    void addDirectory(const char* directory) X_OVERRIDE;
    void addDirectory(const wchar_t* directory) X_OVERRIDE;

    void registerListener(XDirectoryWatcherListener* pListener) X_OVERRIDE;
    void unregisterListener(XDirectoryWatcherListener* pListener) X_OVERRIDE;

    void tick(void);

    X_INLINE bool isDebugEnabled(void) const
    {
        return debug_ != 0;
    }

private:
    X_NO_COPY(XDirectoryWatcher);
    X_NO_ASSIGN(XDirectoryWatcher);

private:
    bool IsRepeat(const core::Path<char>& path);
    bool IsRepeat(const core::Path<wchar_t>& path);

    struct Info_t
    {
        Info_t() :
            mtime(0),
            size(0)
        {
        }
        Info_t(__time64_t time_, int64 size_) :
            mtime(time_),
            size(size_)
        {
        }

        __time64_t mtime;
        int64 size;

        bool operator==(const Info_t& oth) const
        {
            return mtime == oth.mtime && size == oth.size;
        }
    };

    struct WatchInfo
    {
        WatchInfo() :
            directory(0),
            event(0)
        {
            zero_object(result);
            zero_object(overlapped);
        }

        OVERLAPPED overlapped;
        Path<wchar_t> directoryName;
        HANDLE directory;
        HANDLE event;
        char result[4096];
    };

    void notify(Action::Enum action,
        const char* name, const char* oldName, bool isDirectory);

    void checkDirectory(WatchInfo& info);

private:
    typedef core::Array<XDirectoryWatcherListener*> listeners;
    typedef core::Array<WatchInfo> Directorys;

    int debug_;

    Directorys dirs_;
    listeners listeners_;
    Fifo<Info_t> cache_;
};

class XDirectoryWatcherListener
{
public:
    XDirectoryWatcherListener() :
        pMonitor_(nullptr)
    {
    }

    virtual ~XDirectoryWatcherListener()
    {
        if (pMonitor_) {
            pMonitor_->unregisterListener(this);
        }
    }

    // returns true if it action was eaten.
    virtual bool OnFileChange(XDirectoryWatcher::Action::Enum action,
        const char* name, const char* oldName, bool isDirectory) X_ABSTRACT;

protected:
    friend class XDirectoryWatcher;

    void setMonitor(XDirectoryWatcher* pMonitor)
    {
        pMonitor_ = pMonitor;
    }

private:
    XDirectoryWatcher* pMonitor_;
};

X_NAMESPACE_END

#endif // !X_DIRECTORY_WATCHER_H_