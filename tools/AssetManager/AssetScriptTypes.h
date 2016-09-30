#pragma once

#include <qobject.h>


X_NAMESPACE_BEGIN(assman)

#ifdef TEXT
#undef TEXT
#endif // !TEXT



class AssetCheckBoxWidget;
class AssetSpinBoxWidget;
class AssetDoubleSpinBoxWidget;
class AssetPathWidget;
class AssetTextWidget;
class AssetTextureWidget;
class AssetStringWidget;
class AssetColorWidget;
class AssetComboBoxWidget;
class AssetGroupWidget;
class AssetLineEditWidget;

class AssetProperties;

class AssetProps;
class AssetProperty : public QObject
{
	Q_OBJECT

public:
	X_DECLARE_ENUM(PropertyType)(
		UNCLASSIFIED, // has no visual representation.

		CHECKBOX,
		COMBOBOX,
		TEXT,
		STRING,
		LINEEDIT,
		INT,
		BOOL,
		FLOAT,
		COLOR,
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
		UPDATE_ON_CHANGE,
		EDITIABLE,
		NEW_PROP // property was not in input props.
	);

	typedef Flags<Setting> Settings;
	typedef QList<AssetProperty*> ChildrenVec;

	typedef ChildrenVec::ConstIterator ConstIterator;
	typedef ChildrenVec::Iterator Iterator;

	static const int32_t MAX_COL_IDX = 10;


public:
	AssetProperty();
	~AssetProperty();


	void appendGui(QWidget* pParent, QGridLayout* pLayout, int32_t& row, int32_t depth);

	void addRef(void);
	void release(void);
	void clear(void);
	void show(bool vis);
	void enable(bool val);
	void SetModified(bool modified);

	void SetKey(const std::string& key);
	void SetParentKey(const std::string& key);
	void SetType(PropertyType::Enum type);
	void AddChild(AssetProperty* pChild);

	ConstIterator begin(void) const;
	ConstIterator end(void) const;

	void SetTitle(const std::string& title);
	void SetToolTip(const std::string& toolTip);
	void SetLabels(const std::string& labelX, const std::string& labelY,
		const std::string& labelZ, const std::string& labelW);
	void SetIcon(const std::string& icon);
	void SetFontColor(float r, float g, float b);
	void SetBold(bool bold);
	void SetStep(double step);
	void SetEnabled(bool enable);
	void SetVisible(bool vis);
	void ShowJumpToAsset(bool show);
	void UpdateOnChange(bool update);

	void SetInitData(const std::string& val);
	void SetValue(const std::string& val);
	void SetDefaultValue(const std::string& val);
	void SetBool(bool val);
	void SetInt(int32_t val);
	void SetFloat(float val);
	void SetDouble(double val);

	void SetMinMax(int32_t min, int32_t max);
	void SetMinMax(double min, double max);
	void SetEditable(bool canEdit);
	void SetNewProp(void);
	bool isNewProp(void) const;

	PropertyType::Enum GetType(void) const;
	std::string GetKey(void) const;
	std::string GetParentKey(void) const;
	std::string GetTitle(void) const;
	std::string GetToolTip(void) const;
	std::string GetValue(void) const;
	std::string GetDefaultValue(void) const;
	double GetValueFloat(void) const;
	int32_t GetValueInt(void) const;
	bool GetValueBool(void) const;

private slots:
	void valueChanged(const std::string& value);

signals:
	void modified(void);


private:
	static int32_t colSpanForCol(int32_t startCol);

private:
	int32_t refCount_;
	Settings settings_;
	PropertyType::Enum type_;

//	// ordered to try group hot members.
	std::string key_; 
	std::string title_;
	std::string strValue_;
	std::string defaultValue_;
	std::string initData_; // used by combox box.
	std::string toolTip_;
	QString icon_;

	// seperate at bottom to try try improve cache hits for common access members
	std::string parentKey_;

	// used qstring here as not set often
	// and qstring not using short string optermisation so much smaller.
	QString labelsX_;
	QString labelsY_;
	QString labelsZ_;
	QString labelsW_;

	QColor fontCol_;

	QLabel* pLabel_;

	union
	{
		QWidget* pWidget_; // keep?

		AssetCheckBoxWidget* pCheckBoxWidget_;
		AssetSpinBoxWidget* pSpinBoxWidget_;
		AssetDoubleSpinBoxWidget* pDoubleSpinBoxWidget_;
		AssetPathWidget* pPathWidget_;
		AssetTextWidget* pTextWidget_;
		AssetTextureWidget* pTextureWidget_;
		AssetStringWidget* pStringWidget_;
		AssetColorWidget* pColorWidget_;
		AssetComboBoxWidget* pComboBoxWidget_;
		AssetGroupWidget* pGroupWidget_;
		AssetLineEditWidget* pLineEditWidget_;
	};


	// we used these for both real and int min / max
	double min_;
	double max_;
	double step_;
};


class AssetProps : public QObject
{
	typedef QMap<std::string, AssetProperty*> KeyMap;
	typedef QMap<std::string, std::string> KeyValueMap;

public:
	Q_OBJECT

public:
	AssetProps(AssetProperties* pProps);
	~AssetProps();

	AssetProps& operator=(const AssetProps& oth) = default;

	void addRef(void);
	void release(void);
	void clear(void);
	void setAssetType(assetDb::AssetType::Enum type);

	bool parseArgs(const core::string& jsonStr);
	bool extractArgs(core::string& jsonStrOut) const;
	bool appendGui(QGridLayout* pLayout);

	bool isModified(void) const;

public:
	void BeginGroup(const std::string& groupName);

public:
	AssetProperty& addItem(const std::string& key);
	AssetProperty& addItemIU(const std::string& key, AssetProperty::PropertyType::Enum type);

private:
	AssetProperties* pProps_;
	AssetProperty root_;
	AssetProperty* pCur_;

	KeyMap keys_;
	KeyValueMap values_;
	int32_t refCount_;
	int32_t modifiedCount_;
};



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
	AssetScriptProps(AssetProps& props);
	~AssetScriptProps();

	void addRef(void);
	void release(void);


	AssetScriptProperty* AddTexture(const std::string& key, const std::string& default);
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

	void BeginGroup(const std::string& groupName);

	AssetScriptProperty* getItem(const std::string& key);
	std::string getPropValue(const std::string& key);
	double getPropValueFloat(const std::string& key);
	int32_t getPropValueInt(const std::string& key);
	bool getPropValueBool(const std::string& key);

private:
	AssetScriptProperty* getProperty(const std::string& key, AssetProperty::PropertyType::Enum type);

private:
	AssetProps& props_;
	KeyMap map_;
	int32_t refCount_;
};



X_NAMESPACE_END
