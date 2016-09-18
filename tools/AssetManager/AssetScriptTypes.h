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

	AssetProperty& operator=(const AssetProperty& oth);

	void addRef(void);
	void release(void);

	void SetType(PropertyType type);

	AssetProperty& SetTitle(const std::string& title);
	AssetProperty& SetToolTip(const std::string& toolTip);
	AssetProperty& SetEnabled(bool enable);
	AssetProperty& SetVisible(bool enable);

public:
	static AssetProperty* factory(void);
	static AssetProperty* copyFactory(const AssetProperty& oth);


private:
	int32_t refCount_;

	bool enabled_;
	bool visible_;
	bool _pad[2];

	PropertyType type_;
	std::string title_;
	std::string toolTip_;

public:

	std::string strValue_;

	union {
		bool checked;
		float vec[4];

		struct {
			bool checked;
		} CheckBox;

		struct {
			int32_t value;
			int32_t min;
			int32_t max;
		} Int;

		struct {
			float value;
			float min;
			float max;
		} Float;

	};
};


class Asset
{
public:
	Asset();
	~Asset();


	Asset& operator=(const Asset& oth);

	void addRef(void);
	void release(void);

public:
	AssetProperty& AddTexture(const std::string& key, const std::string& default);
	AssetProperty& AddCombo(const std::string& key, const std::string& values);
	AssetProperty& AddCheckBox(const std::string& key, bool default);
	AssetProperty& AddInt(const std::string& key, int32_t default, int32_t min, int32_t max);
	AssetProperty& AddFloat(const std::string& key, float default, float min, float max);

	void BeginGroup(const std::string& groupName);
	void EndGroup(const std::string& groupName);

public:
	static Asset* factory(void);
	static Asset* copyFactory(const Asset& oth);

private:
	AssetProperty& getItem(const std::string& key, AssetProperty::PropertyType type);

private:

	QMap<std::string, AssetProperty> items_;
	int32_t refCount_;
};


X_NAMESPACE_END
