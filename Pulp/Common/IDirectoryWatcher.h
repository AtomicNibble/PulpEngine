#pragma once

#ifndef X_DIRECTORY_WATCHER_I_H_
#define X_DIRECTORY_WATCHER_I_H_


X_NAMESPACE_BEGIN(core)

class XDirectoryWatcherListener;

struct IXDirectoryWatcher
{
	virtual	void addDirectory(const char* directory) X_ABSTRACT;

	virtual void registerListener(XDirectoryWatcherListener* pListener) X_ABSTRACT;
	virtual void unregisterListener(XDirectoryWatcherListener* pListener) X_ABSTRACT;

protected:
	virtual ~IXDirectoryWatcher() {}
};



struct IXHotReload
{
	virtual bool OnFileChange(const char* name) X_ABSTRACT;

protected:
	virtual ~IXHotReload() {}
};


struct IXHotReloadManager
{
	virtual bool addfileType(IXHotReload* pHotReload, const char* extension) X_ABSTRACT;

protected:
	virtual ~IXHotReloadManager() {}
};






X_NAMESPACE_END

#endif // !X_DIRECTORY_WATCHER_I_H_