#pragma once


X_NAMESPACE_BEGIN(assman)

#ifdef TEXT
#undef TEXT
#endif // !TEXT


class AssetProperty
{
public:
	X_DECLARE_ENUM(PropertyType)(
		UNCLASSIFIED, // has no visual representation.

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
		PATH,

		GROUPBOX // used to start a group box.
	);

	X_DECLARE_FLAGS(Setting) (
		ENABLED,
		VISIBLE,
		SHOW_JUMP_TO_ASSET,
		BOLD_TEXT,
		UPDATE_ON_CHANGE
	);

	typedef Flags<Setting> Settings;
	typedef QList<AssetProperty*> ChildrenVec;

	typedef ChildrenVec::ConstIterator ConstIterator;
	typedef ChildrenVec::Iterator Iterator;

public:
	AssetProperty();
	~AssetProperty();

	AssetProperty& operator=(const AssetProperty& oth) = default;

	void appendGui(QGridLayout* pLayout, int32_t& row, int32_t depth, int32_t groupDepth);

	void addRef(void);
	void release(void);
	void clear(void);

	void SetKey(const std::string& key);
	void SetParentKey(const std::string& key);
	void SetType(PropertyType::Enum type);
	void AddChild(AssetProperty* pChild);

	ConstIterator begin(void) const;
	ConstIterator end(void) const;

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
	std::string GetKey(void) const;
	std::string GetParentKey(void) const;
	std::string GetTitle(void) const;
	std::string GetToolTip(void) const;
	std::string GetValue(void) const;
	double GetValueFloat(void) const;
	int32_t GetValueInt(void) const;
	bool GetValueBool(void) const;


private:
	void setLabelText(QLabel* pLabel) const;

	void asGroupBox(QGridLayout* pLayout, int32_t& row, int32_t depth, int32_t groupDepth);
	void asCheckBox(QGridLayout* pLayout, int32_t row, int32_t depth, int32_t groupDepth);
	void asText(QGridLayout* pLayout, int32_t row, int32_t depth, int32_t groupDepth);
	void asIntSpin(QGridLayout* pLayout, int32_t row, int32_t depth, int32_t groupDepth);
	void asFloatSpin(QGridLayout* pLayout, int32_t row, int32_t depth, int32_t groupDepth);


public:
	static AssetProperty* factory(void);
	static AssetProperty* copyFactory(const AssetProperty& oth);

private:
	int32_t refCount_;
	Settings settings_;
	PropertyType::Enum type_;

	// ordered to try group hot members.
	ChildrenVec children_;

	std::string key_; 
	std::string title_;
	std::string strValue_;
	std::string defaultValue_; 
	std::string toolTip_;
	std::string icon_;


	// seperate at bottom to try try improve cache hits for common access members
	std::string parentKey_;

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
	typedef QMap<std::string, AssetProperty*> KeyMap;

public:
	AssetProps();
	~AssetProps();

	AssetProps& operator=(const AssetProps& oth) = default;

	void addRef(void);
	void release(void);
	void clear(void);

	bool parseArgs(const std::string& jsonStr);
	bool extractArgs(std::string& jsonStrOut) const;
	bool appendGui(QGridLayout* pLayout);


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

public:
	AssetProperty& getItem(const std::string& key);
	std::string getPropValue(const std::string& key);
	double getPropValueFloat(const std::string& key);
	int32_t getPropValueInt(const std::string& key);
	bool getPropValueBool(const std::string& key);

private:
	AssetProperty& addItem(const std::string& key, AssetProperty::PropertyType::Enum type);

public:
	static AssetProps* factory(void);
	static AssetProps* copyFactory(const AssetProps& oth);

private:
	AssetProperty root_;
	AssetProperty* pCur_;

	KeyMap keys_;
	int32_t groupDepth_;
	int32_t refCount_;
};


X_NAMESPACE_END
