#pragma once

#include <String\Path.h>
#include <Containers\HashMap.h>

#include <maya\MPxCommand.h>

class PathCmd : public MPxCommand
{
	X_DECLARE_ENUM(Mode)(SET, GET);
	X_DECLARE_ENUM(PathId)(ANIM_OUT, MODEL_OUT);

	static const char* SETTINGS_PATH;

public:
	PathCmd();
	~PathCmd();

	virtual MStatus doIt(const MArgList &args) X_OVERRIDE;

	static void* creator(void);
	static MSyntax newSyntax(void);

private:
	bool SetValue(PathId::Enum id, core::Path<char> value);
	bool GetValue(PathId::Enum id, core::Path<char>& value);

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

