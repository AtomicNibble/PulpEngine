#include "EngineCommon.h"
#include "DirectoryWatcher.h"

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <IFileSys.h>
#include <IConsole.h>

X_NAMESPACE_BEGIN(core)

namespace
{
    template<size_t BufLen>
    bool WatchDirectory(void* directory, char (&result)[BufLen], _OVERLAPPED* overlapped)
    {
        DWORD bytesReturned = 0;

        return ReadDirectoryChangesW(directory,
                   result, BufLen, TRUE,
                   FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                   &bytesReturned, overlapped, 0)
               == TRUE;
    }

    const char* FileActionToStr(DWORD action)
    {
        switch (action) {
            case FILE_ACTION_ADDED:
                return "FILE_ACTION_ADDED";
            case FILE_ACTION_REMOVED:
                return "FILE_ACTION_REMOVED";
            case FILE_ACTION_MODIFIED:
                return "FILE_ACTION_MODIFIED";
            case FILE_ACTION_RENAMED_OLD_NAME:
                return "FILE_ACTION_RENAMED_OLD_NAME";
            case FILE_ACTION_RENAMED_NEW_NAME:
                return "FILE_ACTION_RENAMED_NEW_NAME";
            default:
                break;
        }
        X_ASSERT_UNREACHABLE();
        return "<unknown>";
    }
} // namespace

XDirectoryWatcher::XDirectoryWatcher(core::MemoryArenaBase* arena) :
    cache_(arena),
    debug_(0),
    dirs_(arena),
    listeners_(arena)
{
    X_ASSERT_NOT_NULL(arena);
    cache_.reserve(32);
    dirs_.reserve(4);
}

XDirectoryWatcher::~XDirectoryWatcher(void)
{
    shutDown();
}

void XDirectoryWatcher::init(void)
{
    ADD_CVAR_REF("filesys_dir_watcher_debug", debug_, 0, 0, 1,
        core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Debug messages for directory watcher. 0=off 1=on");
}

void XDirectoryWatcher::shutDown(void)
{
    for (const auto& dirInfo : dirs_) {
        ::CloseHandle(dirInfo.event);
        ::CloseHandle(dirInfo.directory);
    }

    listeners_.clear();
    dirs_.clear();
}

void XDirectoryWatcher::addDirectory(const char* directory)
{
    wchar_t dirW[1024];
    strUtil::Convert(directory, dirW, sizeof(dirW));

    addDirectory(dirW);
}

void XDirectoryWatcher::addDirectory(const wchar_t* directory)
{
    WatchInfo info;

    info.directoryName = Path<wchar_t>(directory);
    info.directoryName.replaceSeprators();
    info.directoryName.ensureSlash();

    info.directory = CreateFileW(
        directory,
        FILE_SHARE_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
        0,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        0);

    if (info.directory == INVALID_HANDLE_VALUE) {
        core::lastError::Description dsc;
        X_ERROR("DirectoryWatcher", "Cannot obtain handle for directory \"%s\". Error: %s", core::lastError::ToString(dsc));
    }

    info.event = CreateEventA(0,
        FALSE, // might try using TRUE see if it helps with duplicate events.
        0,
        0);

    if (info.event == NULL) {
        core::lastError::Description dsc;
        X_ERROR("DirectoryWatcher", "Cannot create event. Error: %s", core::lastError::ToString(dsc));
    }

    info.overlapped.hEvent = info.event;

    // save
    dirs_.append(info);

    // Watch
    WatchDirectory(info.directory, info.result, &info.overlapped);
}

void XDirectoryWatcher::checkDirectory(WatchInfo& info)
{
    char oldFilename[MAX_PATH], filename[MAX_PATH];
    DWORD bytesTransferred = 0;
    size_t offsetToNext = 0;
    size_t offset = 0;

    _FILE_NOTIFY_INFORMATION* pInfo;

    if (GetOverlappedResult(info.directory, &info.overlapped, &bytesTransferred, 0)) {
        if (bytesTransferred > 0) {
            do {
                pInfo = (_FILE_NOTIFY_INFORMATION*)&info.result[offset];

                // null term. (string is not provided with one.)
                pInfo->FileName[(pInfo->FileNameLength >> 1)] = '\0';

                // multibyte me up.
                core::strUtil::Convert(pInfo->FileName, filename);

                // null term.
                filename[(pInfo->FileNameLength >> 1)] = '\0';

                // Path
                core::Path<wchar_t> path(info.directoryName);
                path.append(pInfo->FileName);

#if X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
                X_LOG1_IF(isDebugEnabled(), "DirWatcher", "Action: ^9%s",
                    FileActionToStr(pInfo->Action));
#endif // !X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING

                if (!path.isEmpty()) {
                    bool is_directory = false;

                    if (pInfo->Action != FILE_ACTION_REMOVED) {
                        is_directory = gEnv->pFileSys->isDirectory(path.c_str());
                    }

                    if (is_directory) {
                        switch (pInfo->Action) {
                            case FILE_ACTION_ADDED:
#if X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
                                X_LOG1_IF(isDebugEnabled(), "DirWatcher", "Dir \"%s\" was added", filename);
#endif // !X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
                                notify(Action::ADDED, filename, nullptr, true);
                                break;

                            case FILE_ACTION_MODIFIED:
#if X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
                                X_LOG1_IF(isDebugEnabled(), "DirWatcher", "Dir \"%s\" was modified", filename);
#endif // !X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING

                                notify(Action::MODIFIED, filename, nullptr, true);
                                break;

                                // the old name
                            case FILE_ACTION_RENAMED_OLD_NAME:
                                memcpy(oldFilename, filename, sizeof(oldFilename));
                                break;

                                // then we get given new name, in next loop.
                            case FILE_ACTION_RENAMED_NEW_NAME:
#if X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
                                X_LOG1_IF(isDebugEnabled(), "DirWatcher", "Dir \"%s\" was renamed to \"%s\"",
                                    oldFilename, filename);
#endif // !X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING

                                notify(Action::RENAMED, filename, oldFilename, true);
                                break;
                        }
                    }
                    else {
                        switch (pInfo->Action) {
                            case FILE_ACTION_ADDED:
#if X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
                                X_LOG1_IF(isDebugEnabled(), "DirWatcher", "File \"%s\" was added", filename);
#endif // !X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
                                notify(Action::ADDED, filename, nullptr, false);
                                break;

                            case FILE_ACTION_REMOVED:
#if X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
                                X_LOG1_IF(isDebugEnabled(), "DirWatcher", "File \"%s\" was removed", filename);
#endif // !X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
                                notify(Action::REMOVED, filename, nullptr, false);
                                break;

                            case FILE_ACTION_MODIFIED: {
                                // i want to ignore changes of the same second.
                                if (!IsRepeat(path)) {
#if X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
                                    X_LOG1_IF(isDebugEnabled(), "DirWatcher", "File \"%s\" was modified", filename);
#endif // !X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
                                    notify(Action::MODIFIED, filename, nullptr, false);
                                }
                            } break;

                            // the old name
                            case FILE_ACTION_RENAMED_OLD_NAME:
                                memcpy(oldFilename, filename, sizeof(oldFilename));
                                break;

                                // then we get given new name, in next loop.
                            case FILE_ACTION_RENAMED_NEW_NAME:
#if X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
                                X_LOG1_IF(isDebugEnabled(), "DirWatcher", "File \"%s\" was renamed to \"%s\"", oldFilename, filename);
#endif // !X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
                                notify(Action::RENAMED, filename, oldFilename, false);
                                break;
                        }
                    }
                }

                offsetToNext = pInfo->NextEntryOffset;
                offset += offsetToNext;
            } while (offsetToNext);
        }

        // watch again.
        WatchDirectory(info.directory, info.result, &info.overlapped);
    }
    else {
        if (core::lastError::Get() != ERROR_IO_INCOMPLETE) {
            core::lastError::Description dsc;
            X_ERROR("DirWatcher", "Failed to retrieve directory changes. Error: %s",
                core::lastError::ToString(dsc));
        }
    }
}

void XDirectoryWatcher::tick(void)
{
    X_PROFILE_BEGIN("DirectoryWatcher", core::profiler::SubSys::CORE);
    // check all the directorys.
    Directorys::Iterator it = dirs_.begin();

    for (; it != dirs_.end(); ++it) {
        checkDirectory(*it);
    }
}

bool XDirectoryWatcher::IsRepeat(const core::Path<char>& path)
{
    struct _stat64 st;
    core::zero_object(st);
    _stat64(path.c_str(), &st);

    Info_t info(st.st_mtime, st.st_size);

    Fifo<Info_t>::const_iterator it = cache_.begin();
    for (; it != cache_.end(); ++it) {
        if ((*it) == info) {
            return true;
        }
    }

    if (cache_.capacity() == cache_.size()) {
        cache_.pop();
    }

    cache_.push(info);
    return false;
}

bool XDirectoryWatcher::IsRepeat(const core::Path<wchar_t>& path)
{
    struct _stat64 st;
    core::zero_object(st);
    _wstat64(path.c_str(), &st);

    Info_t info(st.st_mtime, st.st_size);

    Fifo<Info_t>::const_iterator it = cache_.begin();
    for (; it != cache_.end(); ++it) {
        if ((*it) == info) {
            return true;
        }
    }

    if (cache_.capacity() == cache_.size()) {
        cache_.pop();
    }

    cache_.push(info);
    return false;
}

void XDirectoryWatcher::registerListener(IDirectoryWatcherListener* pListener)
{
    listeners_.append(X_ASSERT_NOT_NULL(pListener));
}

void XDirectoryWatcher::unregisterListener(IDirectoryWatcherListener* pListener)
{
    listeners_.removeIndex(listeners_.find(X_ASSERT_NOT_NULL(pListener)));
}

void XDirectoryWatcher::notify(Action::Enum action,
    const char* pName, const char* pOldName, bool isDirectory)
{
    auto it = listeners_.begin();
    for (; it != listeners_.end(); ++it) {
        if ((*it)->OnFileChange(action, pName, pOldName, isDirectory)) {
#if X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
            X_LOG1_IF(isDebugEnabled(), "DirWatcher", "Event was handled");
#endif // !X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
            break;
        }
    }

#if X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
    if (it == listeners_.end()) {
        X_LOG1_IF(isDebugEnabled(), "DirWatcher", "Event was NOT handled");
    }
#endif // !X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
}

X_NAMESPACE_END