#pragma once

#include <String\Path.h>
#include <Containers\HashMap.h>

#include <maya\MPxCommand.h>

class PathCache
{
	static const char* SETTINGS_PATH;

public:
	X_DECLARE_ENUM(PathId)(ANIM_OUT, MODEL_OUT);

public:
	PathCache();
	~PathCache();

	static void Init(void);
	static void ShutDown(void);

public:
	bool SetValue(PathId::Enum id, core::Path<char> value);
	bool GetValue(PathId::Enum id, core::Path<char>& value);

private:
	const char* PathIdToStr(PathId::Enum id);

	bool ReloadCache(void);
	bool FlushCache(void);

	core::Path<char> GetSettingsPath(void);

private:
	typedef core::HashMap<core::StackString<64>, core::Path<char>> PathCacheMap;

	bool cacheLoaded_;
	bool _pad_[3];

	PathCacheMap PathCache_;
};


class PathCmd : public MPxCommand
{
	X_DECLARE_ENUM(Mode)(SET, GET);
	typedef PathCache::PathId PathId;

public:
	PathCmd();
	~PathCmd();

	virtual MStatus doIt(const MArgList &args) X_OVERRIDE;

	static void* creator(void);
	static MSyntax newSyntax(void);
};

