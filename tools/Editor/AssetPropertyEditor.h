#pragma once

#include <QObject>
#include "IEditor.h"
#include "IAssetEntry.h"



#ifdef TEXT
#undef TEXT
#endif // !TEXT


X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(editor)

class AssetPropertyEditor;
class AssetPropertyEditorWidget;
class AssetPropsScriptManager;

class AssetCheckBoxWidget;
class AssetSpinBoxWidget;
class AssetDoubleSpinBoxWidget;
class AssetPathWidget;
class AssetTextWidget;
class AssetFontWidget;
class AssetTextureWidget;
class AssetModelWidget;
class AssetAnimWidget;
class AssetAssetRefWidget;
class AssetStringWidget;
class AssetColorWidget;
class AssetComboBoxWidget;
class AssetGroupWidget;
class AssetLineEditWidget;
class AssetProperties;


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

		FONT,
		IMAGE,
		MODEL,
		ANIM,
		ASSET_REF,
		PATH,
		LABEL,

		GROUPBOX // used to start a group box.
	);

	X_DECLARE_FLAGS(Setting) (
		ENABLED,
		VISIBLE,
		SHOW_JUMP_TO_ASSET,
		BOLD_TEXT,
		UPDATE_ON_CHANGE,
		EDITIABLE,
		MODIFIED,
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


	void appendGui(assetDb::AssetDB& db, IAssetEntry* pAssEntry, QWidget* pParent, QGridLayout* pLayout, int32_t& row, int32_t depth);

	void addRef(void);
	void release(void);
	void clear(void);
	void clearUI(void);
	void show(bool vis);
	void enable(bool val);
	void SetModified(bool modified);

	void collapseAll(void);
	void expandAll(void);

	void SetKey(const std::string& key);
	void SetParentKey(const std::string& key);
	void SetType(PropertyType::Enum type);
	void AddChild(AssetProperty* pChild);
	bool HasChild(const AssetProperty& prop) const;
	bool RemoveChild(const AssetProperty& prop);

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
	void SetSavedValue(const std::string& val);
	void SetDefaultValue(const std::string& val);

	void SetMinMax(int32_t min, int32_t max);
	void SetMinMax(double min, double max);
	void SetEditable(bool canEdit);
	void SetNewProp(void);

	bool isNewProp(void) const;
	bool isModified(void) const;
	bool isUpdateOnChange(void) const;
	bool isVisible(void) const;
	PropertyType::Enum GetType(void) const;
	const std::string& GetKey(void) const;
	const std::string& GetParentKey(void) const;
	const std::string& GetTitle(void) const;
	const std::string& GetToolTip(void) const;
	const std::string& GetValue(void) const;
	const std::string& GetDefaultValue(void) const;
	double GetValueFloat(void) const;
	int32_t GetValueInt(void) const;
	bool GetValueBool(void) const;

private:
	void setModifiedStyle(bool modified);

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
	std::string strValue_;			// the current value
	std::string strSavedValue_;		// the value from DB
	std::string defaultValue_;		// default value is set by the ui script
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
		AssetFontWidget* pFontWidget_;
		AssetTextureWidget* pTextureWidget_;
		AssetModelWidget* pModelWidget_;
		AssetAnimWidget* pAnimWidget_;
		AssetAssetRefWidget* pAssetRefWidget_;
		AssetStringWidget* pStringWidget_;
		AssetColorWidget* pColorWidget_;
		AssetComboBoxWidget* pComboBoxWidget_;
		AssetGroupWidget* pGroupWidget_;
		AssetLineEditWidget* pLineEditWidget_;
		QLabel* pLabelWidget_;
	};


	// we used these for both real and int min / max
	double min_;
	double max_;
	double step_;
};


class AssetProperties : public IAssetEntry
{
	Q_OBJECT

	typedef QMap<std::string, AssetProperty*> KeyMap;
	typedef QMap<std::string, std::string> KeyValueMap;


public:
	AssetProperties(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, AssetPropertyEditorWidget* widget);
	virtual ~AssetProperties();

	void clear(void);

	void setAssetType(assetDb::AssetType::Enum type);
	void setWidget(QWidget* widget);

	// IAssetEntry
	bool save(QString& errorString) X_OVERRIDE;
	bool updateRawFile(const ByteArr& compressedData) X_OVERRIDE;
	bool getRawFile(ByteArr& rawData) X_OVERRIDE;
	bool updateThumb(const ByteArr& data, Vec2i thumbDim, Vec2i srcDim,
		core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl) X_OVERRIDE;
	bool getThumb(ByteArr& data, Vec2i& dim) X_OVERRIDE;

	bool reloadUi(void) X_OVERRIDE;


	bool isModified(void) const X_OVERRIDE;
	bool isSaveAsAllowed(void) const X_OVERRIDE;
	// ~IAssetEntry

	void collapseAll(void);
	void expandAll(void);

	bool loadProps(QString& errorString, const QString& fileName, assetDb::AssetType::Enum type);

private:
	bool parseArgs(const core::string& jsonStr);
	bool extractArgs(core::string& jsonStrOut) const;
	bool appendGui(QWidget* pParent, QGridLayout* pLayout);
	void setNotModifiedModified(void);


public:
	void BeginGroup(const std::string& groupName);

private:
	AssetProperty& addItem(const std::string& key);
public:
	AssetProperty& addItemIU(const std::string& key, AssetProperty::PropertyType::Enum type);
	void showInCurrentCat(const AssetProperty& prop);

private slots:
	void propModified(void);
	void reloadUiSlot(void);

signals:
	void modificationChanged(bool);

private:
	AssetPropertyEditorWidget* getEditor(void);

private:
	assetDb::AssetDB& db_;

private:
	AssetPropsScriptManager* pPropScriptMan_;
	AssetPropertyEditorWidget* pWidget_;
	QWidget* pCon_;
	QGridLayout* pLayout_;
private:
	AssetProperty root_;
	AssetProperty* pCur_;

	KeyMap keys_;
	KeyValueMap values_;
};


class AssetPropertyEditorWidget : public QScrollArea
{
	Q_OBJECT
public:
	AssetPropertyEditorWidget(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, QWidget *parent = nullptr);
	AssetPropertyEditorWidget(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, AssetProperties* pAssetEntry, QWidget *parent = nullptr);
	AssetPropertyEditorWidget(AssetPropertyEditorWidget* pOther);
	~AssetPropertyEditorWidget();

private:
	void ctor(const QSharedPointer<AssetProperties>& props);

public:
	bool open(QString* pErrorString, const QString& data, assetDb::AssetType::Enum type);

	bool isUndoAvailable(void) const;
	bool isRedoAvailable(void) const;
	bool isReadOnly(void) const;

	void undo(void);
	void redo(void);
	void copy(void);
	void cut(void);
	void paste(void);

	void collapseAll(void);
	void expandAll(void);


	AssetPropertyEditor* editor(void);
	AssetProperties* assetProperties(void) const;

protected:
	AssetPropertyEditor* createEditor(void);

	void contextMenuEvent(QContextMenuEvent* e);
	void showDefaultContextMenu(QContextMenuEvent* e, const Id menuContextId);

	void appendMenuActionsFromContext(QMenu *menu, const Id menuContextId);
	void appendStandardContextMenuActions(QMenu *menu);

signals:
	void undoAvailable(bool);
	void readOnlyChanged(void);
	void modificationChanged(bool);
	void copyAvailable(bool);

private:
	assetDb::AssetDB& db_;
	AssetPropsScriptManager* pPropScriptMan_;
	AssetPropertyEditor* pEditor_;
	QSharedPointer<AssetProperties> assetProps_;
};


class AssetPropertyEditor : public IEditor
{
	Q_OBJECT
public:
	AssetPropertyEditor(AssetPropertyEditorWidget* editorWidget);
	~AssetPropertyEditor();

	bool open(QString* pErrorString, const QString& fileName, assetDb::AssetType::Enum type) X_OVERRIDE;
	IAssetEntry* assetEntry(void) X_OVERRIDE;
	Id id(void) const X_OVERRIDE;

	bool duplicateSupported(void) const X_OVERRIDE;
	IEditor* duplicate(void) X_OVERRIDE;

private slots:
	void modificationChanged(bool);

private:
	AssetPropertyEditorWidget* pEditorWidget_;
};



X_NAMESPACE_END