#pragma once

#ifndef X_DIRECTORY_WATCHER_H_
#define X_DIRECTORY_WATCHER_H_

#include "String\Path.h"

#include "Containers\Fifo.h"
#include "Containers\Array.h"

#include <set>

#include <IDirectoryWatcher.h>

X_NAMESPACE_BEGIN(core)

class XDirectoryWatcher : public IDirectoryWatcher
{
public:
    using IDirectoryWatcher::Action;

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

private:
    typedef core::Array<IDirectoryWatcherListener*> ListenersArr;
    typedef core::Array<WatchInfo> Directorys;

    X_NO_COPY(XDirectoryWatcher);
    X_NO_ASSIGN(XDirectoryWatcher);

public:
    explicit XDirectoryWatcher(core::MemoryArenaBase* arena);
    ~XDirectoryWatcher(void);

    void Init(void);
    void ShutDown(void);

    void addDirectory(const char* pDirectory) X_OVERRIDE;
    void addDirectory(const wchar_t* pDirectory) X_OVERRIDE;

    void registerListener(IDirectoryWatcherListener* pListener) X_OVERRIDE;
    void unregisterListener(IDirectoryWatcherListener* pListener) X_OVERRIDE;

    void tick(void);

    X_INLINE bool isDebugEnabled(void) const;

private:
    bool IsRepeat(const core::Path<char>& path);
    bool IsRepeat(const core::Path<wchar_t>& path);

    void notify(Action::Enum action, const char* pName, const char* pOldName, bool isDirectory);

    void checkDirectory(WatchInfo& info);

private:
    int debug_;

    Directorys dirs_;
    ListenersArr listeners_;
    Fifo<Info_t> cache_;
};

X_INLINE bool XDirectoryWatcher::isDebugEnabled(void) const
{
    return debug_ != 0;
}

X_NAMESPACE_END

#endif // !X_DIRECTORY_WATCHER_H_