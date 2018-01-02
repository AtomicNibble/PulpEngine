#pragma once

#include <qobject.h>

#include "AssetPropertyEditor.h"

X_NAMESPACE_DECLARE(engine, 
	namespace techset
	{
		class TechSetDefs;
	} // namespace techset
);

X_NAMESPACE_BEGIN(assman)

// ----------------------------------------------

class AssetScriptProperty
{
public:
	AssetScriptProperty();
	AssetScriptProperty(AssetProperty* prop);
	AssetScriptProperty(const AssetScriptProperty& oth);
	~AssetScriptProperty();

	AssetScriptProperty& operator=(const AssetScriptProperty& oth);

	AssetProperty& prop(void);

	void addRef(void);
	void release(void);

	AssetScriptProperty* SetTitle(const std::string& title);
	AssetScriptProperty* SetToolTip(const std::string& toolTip);
	AssetScriptProperty* SetLabels(const std::string& labelX, const std::string& labelY,
		const std::string& labelZ, const std::string& labelW);
	AssetScriptProperty* SetIcon(const std::string& icon);
	AssetScriptProperty* SetFontColor(float r, float g, float b);
	AssetScriptProperty* SetBold(bool bold);
	AssetScriptProperty* SetStep(double step);
	AssetScriptProperty* SetEnabled(bool enable);
	AssetScriptProperty* SetVisible(bool vis);
	AssetScriptProperty* ShowJumpToAsset(bool show);
	AssetScriptProperty* UpdateOnChange(bool update);

	AssetScriptProperty* SetValue(const std::string& val);
	AssetScriptProperty* SetDefaultValue(const std::string& val);
	AssetScriptProperty* SetBool(bool val);
	AssetScriptProperty* SetInt(int32_t val);
	AssetScriptProperty* SetFloat(float val);
	AssetScriptProperty* SetDouble(double val);

	std::string GetTitle(void) const;
	std::string GetToolTip(void) const;
	std::string GetValue(void) const;
	double GetValueFloat(void) const;
	int32_t GetValueInt(void) const;
	bool GetValueBool(void) const;

public:
	static AssetScriptProperty* factory(void);
	static AssetScriptProperty* copyFactory(const AssetScriptProperty& oth);

private:
	AssetProperty* pProp_;
	int32_t refCount_;
};


class AssetScriptProps
{
	typedef QMap<std::string, AssetScriptProperty*> KeyMap;

public:
	AssetScriptProps(AssetProperties& props, engine::techset::TechSetDefs& techDefs);
	~AssetScriptProps();

	void addRef(void);
	void release(void);


	AssetScriptProperty* AddFont(const std::string& key, const std::string& default);
	AssetScriptProperty* AddTexture(const std::string& key, const std::string& default);
	AssetScriptProperty* AddModel(const std::string& key, const std::string& default);
	AssetScriptProperty* AddAnim(const std::string& key, const std::string& default);
	AssetScriptProperty* AddAssetRef(const std::string& key, const std::string& type);
	AssetScriptProperty* AddCombo(const std::string& key, const std::string& values, bool editiable = false);
	AssetScriptProperty* AddCheckBox(const std::string& key, bool default);
	AssetScriptProperty* AddInt(const std::string& key, int32_t default, int32_t min, int32_t max);
	AssetScriptProperty* AddColor(const std::string& key, double r, double g, double b, double a);
	AssetScriptProperty* AddFloat(const std::string& key, double default, double min, double max);
	AssetScriptProperty* AddVec2(const std::string& keyX, const std::string& keyY,
		double x, double y, double min, double max);
	AssetScriptProperty* AddVec3(const std::string& keyX, const std::string& keyY, const std::string& keyZ,
		double x, double y, double z, double min, double max);
	AssetScriptProperty* AddVec4(const std::string& keyX, const std::string& keyY, const std::string& keyZ, const std::string& keyW,
		double x, double y, double z, double w, double min, double max);
	AssetScriptProperty* AddText(const std::string& key, const std::string& value);
	AssetScriptProperty* AddString(const std::string& key, const std::string& value);
	AssetScriptProperty* AddPath(const std::string& key, const std::string& value);
	AssetScriptProperty* AddLabel(const std::string& key, const std::string& value);

	void BeginGroup(const std::string& groupName);

	AssetScriptProperty* getItem(const std::string& key);
	std::string getPropValue(const std::string& key);
	double getPropValueFloat(const std::string& key);
	int32_t getPropValueInt(const std::string& key);
	bool getPropValueBool(const std::string& key);
	void showProp(const std::string& key);

	std::string getMaterialCats(void);
	std::string getMaterialTypes(std::string& cat);
	bool isMaterialType(std::string& cat, std::string& type);
	void addMaterialTypeProps(std::string& cat, std::string& type);

private:
	AssetScriptProperty* getProperty(const std::string& key, AssetProperty::PropertyType::Enum type);

private:
	AssetProperties& props_;
	engine::techset::TechSetDefs& techDefs_;

	KeyMap map_;
	int32_t refCount_;
};



X_NAMESPACE_END
