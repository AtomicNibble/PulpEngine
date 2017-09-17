#pragma once

#include <String\Path.h>
#include <Containers\HashMap.h>

#include <maya\MPxCommand.h>

X_NAMESPACE_BEGIN(maya)

class SettingsCache
{
	static const char* SETTINGS_PATH;

public:
	X_DECLARE_ENUM(SettingId)(ANIM_OUT, MODEL_OUT, EXPORTMODE, SAVE_IF_CHANGED_MODE);

public:
	SettingsCache();
	~SettingsCache();

	static void Init(void);
	static void ShutDown(void);

public:
	bool SetValue(SettingId::Enum id, const core::StackString512& value);
	bool GetValue(SettingId::Enum id, core::StackString512& value);

private:
	const char* SetIdToStr(SettingId::Enum id);

	bool ReloadCache(void);
	bool FlushCache(void);

	core::Path<char> GetSettingsPath(void);

private:
	typedef core::HashMap<core::StackString<64>, core::StackString<512,char>> SettingsCacheMap;

	bool cacheLoaded_;
	bool _pad_[3];

	SettingsCacheMap settingsCache_;
};


class SettingsCmd : public MPxCommand
{
	X_DECLARE_ENUM(Mode)(SET, GET);
	typedef SettingsCache::SettingId SettingId;

public:
	SettingsCmd();
	~SettingsCmd();

	virtual MStatus doIt(const MArgList &args) X_OVERRIDE;

	static void* creator(void);
	static MSyntax newSyntax(void);
};

X_NAMESPACE_END