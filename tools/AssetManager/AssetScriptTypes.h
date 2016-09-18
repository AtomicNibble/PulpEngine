#pragma once


X_NAMESPACE_BEGIN(assman)

#ifdef TEXT
#undef TEXT
#endif // !TEXT


class AssetProperty
{
public:
	X_DECLARE_ENUM(PropertyType)(
		CHECKBOX,
		COMBOBOX,
		TEXT,
		INT,
		BOOL,
		FLOAT,
		VEC2,
		VEC3,
		VEC4,

		IMAGE,
		PATH
	);

	X_DECLARE_FLAGS(Setting) (
		ENABLED,
		VISIBLE,
		SHOW_JUMP_TO_ASSET,
		BOLD_TEXT,
		UPDATE_ON_CHANGE
	);

	typedef Flags<Setting> Settings;

public:
	AssetProperty();
	~AssetProperty();

	AssetProperty& operator=(const AssetProperty& oth) = default;

	void addRef(void);
	void release(void);

	void SetPropertyName(const std::string& property);
	void SetType(PropertyType::Enum type);

	AssetProperty& SetTitle(const std::string& title);
	AssetProperty& SetToolTip(const std::string& toolTip);
	AssetProperty& SetLabels(const std::string& labelX, const std::string& labelY,
		const std::string& labelZ, const std::string& labelW);
	AssetProperty& SetIcon(const std::string& icon);
	AssetProperty& SetFontColor(float r, float g, float b);
	AssetProperty& SetBold(bool bold);
	AssetProperty& SetStep(double step);
	AssetProperty& SetEnabled(bool enable);
	AssetProperty& SetVisible(bool vis);
	AssetProperty& ShowJumpToAsset(bool show);
	AssetProperty& UpdateOnChange(bool update);

	AssetProperty& SetValue(const std::string& val);
	AssetProperty& SetDefaultValue(const std::string& val);
	AssetProperty& SetBool(bool val);
	AssetProperty& SetInt(int32_t val);
	AssetProperty& SetFloat(float val);
	AssetProperty& SetDouble(double val);

	void SetMinMax(int32_t min, int32_t max);
	void SetMinMax(double min, double max);

	PropertyType::Enum GetType(void) const;
	std::string GetTitle(void) const;
	std::string GetToolTip(void) const;
	std::string GetValue(void) const;
	double GetValueFloat(void) const;
	int32_t GetValueInt(void) const;
	bool GetValueBool(void) const;

public:
	static AssetProperty* factory(void);
	static AssetProperty* copyFactory(const AssetProperty& oth);

private:
	int32_t refCount_;
	Settings settings_;

	PropertyType::Enum type_;
	std::string property_;

	std::string title_;
	std::string toolTip_;
	std::string icon_;

	std::string defaultValue_; 
	std::string strValue_;
	std::string strValueY_;
	std::string strValueZ_;
	std::string strValueW_;

	std::string labelsX_;
	std::string labelsY_;
	std::string labelsZ_;
	std::string labelsW_;

	QColor fontCol_;

	// we used these for both real and int min / max
	double min_;
	double max_;
	double step_;
};


class AssetProps
{
public:
	AssetProps();
	~AssetProps();


	AssetProps& operator=(const AssetProps& oth) = default;

	void addRef(void);
	void release(void);

	bool createGui(QWidget* pParent);

public:
	AssetProperty& AddTexture(const std::string& key, const std::string& default);
	AssetProperty& AddCombo(const std::string& key, const std::string& values);
	AssetProperty& AddCheckBox(const std::string& key, bool default);
	AssetProperty& AddInt(const std::string& key, int32_t default, int32_t min, int32_t max);
	AssetProperty& AddFloat(const std::string& key, double default, double min, double max);
	AssetProperty& AddVec2(const std::string& keyX, const std::string& keyY,
		double x, double y, double min, double max);
	AssetProperty& AddVec3(const std::string& keyX, const std::string& keyY, const std::string& keyZ, 
		double x, double y, double z, double min, double max);
	AssetProperty& AddVec4(const std::string& keyX, const std::string& keyY, const std::string& keyZ, const std::string& keyW,
		double x, double y, double z, double w, double min, double max);
	AssetProperty& AddText(const std::string& key, const std::string& value);
	AssetProperty& AddPath(const std::string& key, const std::string& value);

	void BeginGroup(const std::string& groupName);
	void EndGroup(const std::string& groupName);

public:
	AssetProperty& getItem(const std::string& key);
	std::string getPropValue(const std::string& key);
	double getPropValueFloat(const std::string& key);
	int32_t getPropValueInt(const std::string& key);
	bool getPropValueBool(const std::string& key);


public:
	static AssetProps* factory(void);
	static AssetProps* copyFactory(const AssetProps& oth);

private:
	AssetProperty& addItem(const std::string& key, AssetProperty::PropertyType::Enum type);

private:

	QList<AssetProperty> items_;
	QMap<std::string, int32_t> itemsLookup_;

	int32_t refCount_;
};


X_NAMESPACE_END
