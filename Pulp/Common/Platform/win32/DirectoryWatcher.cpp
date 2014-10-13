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
	bool WatchDirectory(void *directory, char(&result)[BufLen], _OVERLAPPED *overlapped)
	{
		DWORD bytesReturned = 0;

		return ReadDirectoryChangesW(directory,
			result, BufLen, TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
			&bytesReturned, overlapped, 0
			) == TRUE;
	}

}



XDirectoryWatcher::XDirectoryWatcher(core::MemoryArenaBase* arena) :
m_cache(arena),
m_debug(0),
m_dirs(arena),
m_listeners(arena)
{
	X_ASSERT_NOT_NULL(arena);
	m_cache.reserve(32);
	m_dirs.reserve(4);
}


XDirectoryWatcher::~XDirectoryWatcher(void)
{
	ShutDown();
}

void XDirectoryWatcher::Init(void)
{
	ADD_CVAR_REF("filesys_dir_watcher_debug", m_debug, 0, 0, 1, core::VarFlag::SYSTEM,
		"Debug messages for directory watcher. 0=off 1=on");



}

void XDirectoryWatcher::ShutDown(void)
{
	// clear the monitors on listeners, to prevent them trying to access
	// invalid memory.
	listeners::Iterator it;
	for (it = m_listeners.begin(); it != m_listeners.end(); ++it)
	{
		XDirectoryWatcherListener* pListener = *it;

		if (pListener) {
			pListener->setMonitor(nullptr);
		}
	}

	Directorys::Iterator it2;
	for (it2 = m_dirs.begin(); it2 != m_dirs.end(); ++it2)
	{
		WatchInfo* pInfo = it2;

		CloseHandle(pInfo->event);
		CloseHandle(pInfo->directory);

	}

	m_listeners.clear();
	m_dirs.clear();
}



void XDirectoryWatcher::addDirectory(const char* directory)
{
	WatchInfo info;

	info.directoryName = Path(directory);
	info.directoryName.replaceSeprators();
	info.directoryName.ensureSlash();

	info.directory = CreateFileA(
		directory,
		FILE_SHARE_READ,
		FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
		0,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
		0
	);


	if (info.directory == INVALID_HANDLE_VALUE)
	{
		core::lastError::Description dsc;
		X_ERROR("DirectoryWatcher", "Cannot obtain handle for directory \"%s\". Error: %s", core::lastError::ToString(dsc));
	}

	info.event = CreateEventA(0, 
		FALSE, // might try using TRUE see if it helps with duplicate events. 
		0, 
		0);

	if (info.event == NULL)
	{
		core::lastError::Description dsc;
		X_ERROR("DirectoryWatcher", "Cannot create event. Error: %s", core::lastError::ToString(dsc));
	}

	info.overlapped.hEvent = info.event;

	// save
	m_dirs.append(info);

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

	if (GetOverlappedResult(info.directory, &info.overlapped, &bytesTransferred, 0))
	{
		do
		{
			pInfo = (_FILE_NOTIFY_INFORMATION*)&info.result[offset];
			
			// null term. (string is not provided with one.)
			pInfo->FileName[(pInfo->FileNameLength >> 1)] = '\0';


			// multibyte me up.
			core::strUtil::Convert(pInfo->FileName, filename);

			// null term.
			filename[(pInfo->FileNameLength >> 1)] = '\0';


			// Path
			core::Path path(info.directoryName);
			path.append(filename);


			if (!path.isEmpty())
			{

				bool is_directory = false;

				if (pInfo->Action != FILE_ACTION_REMOVED)
					is_directory = gEnv->pFileSys->directoryExists(path.c_str());

				if (is_directory)
				{
					switch (pInfo->Action)
					{
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
							X_LOG1_IF(isDebugEnabled(), "DirWatcher", "Dir \"%s\" was renamed to \"%s\"", oldFilename, filename);
#endif // !X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING

							notify(Action::RENAMED, filename, oldFilename, true);
							break;
					}
				}
				else
				{
					switch (pInfo->Action)
					{
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

						case FILE_ACTION_MODIFIED:
						{
							// i want to ignore changes of the same second.
							if (!IsRepeat(path))
							{
#if X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
								X_LOG1_IF(isDebugEnabled(), "DirWatcher", "File \"%s\" was modified", filename);
#endif // !X_DEBUG || X_ENABLE_DIR_WATCHER_LOGGING
								notify(Action::MODIFIED, filename, nullptr, false);
							}
						}
							break;

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
		}
		while (offsetToNext);

		// watch again.
		WatchDirectory(info.directory, info.result, &info.overlapped);
	}
	else
	{
		if (core::lastError::Get() != ERROR_IO_INCOMPLETE)
		{
			core::lastError::Description dsc;
			X_ERROR("DirWatcher", "Failed to retrieve directory changes. Error: %s",
				core::lastError::ToString(dsc));
		}
	}
}

void XDirectoryWatcher::tick(void)
{
	// check all the directorys.
	Directorys::Iterator it = m_dirs.begin();

	for (; it != m_dirs.end(); ++it)
	{
		checkDirectory(*it);
	}
}

bool XDirectoryWatcher::IsRepeat(const core::Path& path)
{
	struct _stat64 st;
	core::zero_object(st);
	_stat64(path.c_str(), &st);

	Info_t info(st.st_mtime, st.st_size);

	Fifo<Info_t>::const_iterator it = m_cache.begin();

	for (; it != m_cache.end(); ++it)
	{
		if ((*it) == info)
			return true;
	}

	if (m_cache.capacity() == m_cache.size())
		m_cache.pop();

	m_cache.push(info);
	return false;
}



void XDirectoryWatcher::registerListener(XDirectoryWatcherListener* pListener)
{
	X_ASSERT_NOT_NULL(pListener);
	pListener->setMonitor(this);
	m_listeners.insert(pListener);
}

void XDirectoryWatcher::unregisterListener(XDirectoryWatcherListener* pListener)
{
	X_ASSERT_NOT_NULL(pListener);
	pListener->setMonitor(nullptr);
	m_listeners.removeIndex(m_listeners.find(pListener));
}

void XDirectoryWatcher::notify(Action::Enum action,
	const char* name, const char* oldName, bool isDirectory)
{
	listeners::ConstIterator it = m_listeners.begin();
	for (; it != m_listeners.end(); ++it) {
		(*it)->OnFileChange(action, name, oldName, isDirectory);
		//	break;
	}
}


X_NAMESPACE_END