#pragma once


X_NAMESPACE_BEGIN(assman)



class AssetProperty
{
public:
	enum class PropertyType
	{
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
	};

public:
	AssetProperty();
	~AssetProperty();

	AssetProperty& operator=(const AssetProperty& oth) = default;

	void addRef(void);
	void release(void);

	void SetPropertyName(const std::string& property);
	void SetType(PropertyType type);

	AssetProperty& SetTitle(const std::string& title);
	AssetProperty& SetToolTip(const std::string& toolTip);
	AssetProperty& SetIcon(const std::string& icon);
	AssetProperty& SetFontColor(float r, float g, float b);
	AssetProperty& SetBold(bool bold);
	AssetProperty& SetStep(double step);
	AssetProperty& SetEnabled(bool enable);
	AssetProperty& SetVisible(bool vis);

	AssetProperty& SetValue(const std::string& val);
	AssetProperty& SetBool(bool val);
	AssetProperty& SetInt(int32_t val);
	AssetProperty& SetFloat(float val);

	std::string GetTitle(void) const;
	std::string GetToolTip(void) const;
	std::string GetValue(void) const;
	bool GetBool(void) const;


public:
	static AssetProperty* factory(void);
	static AssetProperty* copyFactory(const AssetProperty& oth);


private:
	int32_t refCount_;

	bool enabled_;
	bool visible_;
	bool checked_;
	bool boldText_;

	PropertyType type_;
	std::string property_;
	std::string title_;
	std::string toolTip_;
	std::string icon_;
	std::string strValue_;

	QColor fontCol_;
	double step_;

public:

	union {

		struct {
			bool checked;
		} CheckBox;

		struct {
			int32_t value;
			int32_t min;
			int32_t max;
		} Int;

		struct {
			double value;
			double min;
			double max;
		} Float;

		struct {
			double x, y, z, w;
			double min;
			double max;
		} Vec;
	};
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
	AssetProperty& AddVec2(const std::string& key, double x, double y, double min, double max);
	AssetProperty& AddVec3(const std::string& key, double x, double y, double z, double min, double max);
	AssetProperty& AddVec4(const std::string& key, double x, double y, double z, double w, double min, double max);
	AssetProperty& AddText(const std::string& key, const std::string& value);
	AssetProperty& AddPath(const std::string& key, const std::string& value);


	void BeginGroup(const std::string& groupName);
	void EndGroup(const std::string& groupName);

public:
	static AssetProps* factory(void);
	static AssetProps* copyFactory(const AssetProps& oth);

private:
	AssetProperty& getItem(const std::string& key, AssetProperty::PropertyType type);

private:

	QList<AssetProperty> items_;
	QMap<std::string, int32_t> itemsLookup_;

	int32_t refCount_;
};


X_NAMESPACE_END
