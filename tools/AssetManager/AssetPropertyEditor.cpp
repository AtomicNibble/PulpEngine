#include "stdafx.h"
#include "AssetPropertyEditor.h"

#include "AssetScript.h"
#include "AssetScriptTypes.h"

#include "AssetColorWidget.h"
#include "AssetComboBoxWidget.h"
#include "AssetCheckBoxWidget.h"
#include "AssetSpinBoxWidget.h"
#include "AssetStringWidget.h"
#include "AssetLineEditWidget.h"
#include "AssetTextWidget.h"
#include "AssetTextureWidget.h"
#include "AssetGroupWidget.h"
#include "AssetPathWidget.h"

#include <String\Json.h>
#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(assman)



namespace
{
	template<typename TStr>
	void tokenize(std::vector<TStr>& tokensOut, const TStr& str, const TStr& delimiter)
	{
		TStr::size_type start = 0, end = 0;

		tokensOut.clear();

		while (end != TStr::npos)
		{
			end = str.find(delimiter, start);

			TStr::size_type subEnd = (end == TStr::npos) ? TStr::npos : end - start;
			tokensOut.push_back(str.substr(start, subEnd));

			start = ((end > (TStr::npos - delimiter.size()))
				? TStr::npos : end + delimiter.size());
		}
	}

} // namespace



AssetProperty::AssetProperty() :
	refCount_(1),
	min_(std::numeric_limits<float>::min()),
	max_(std::numeric_limits<float>::max()),
	step_(0.0001),
	type_(PropertyType::UNCLASSIFIED),
	pLabel_(nullptr),
	pWidget_(nullptr)
{
	settings_.Set(Setting::VISIBLE);
	settings_.Set(Setting::ENABLED);
}

AssetProperty::~AssetProperty()
{
	clear();
}

void AssetProperty::appendGui(QWidget* pParent, QGridLayout* pLayout, int32_t& row, int32_t depth)
{
	const std::string& val = strValue_;

	switch (type_)
	{
	case PropertyType::CHECKBOX:
		pCheckBoxWidget_ = new AssetCheckBoxWidget(pParent, val);
		break;
	case PropertyType::INT:
		pSpinBoxWidget_ = new AssetSpinBoxWidget(pParent, val, min_, max_, step_);
		break;
	case PropertyType::FLOAT:
		pDoubleSpinBoxWidget_ = new AssetDoubleSpinBoxWidget(pParent, val, min_, max_, step_);
		break;
	case PropertyType::PATH:
		pPathWidget_ = new AssetPathWidget(pParent, val);
		break;
	case PropertyType::TEXT:
		pTextWidget_ = new AssetTextWidget(pParent, val);
		break;
	case PropertyType::LINEEDIT:
		pLineEditWidget_ = new AssetLineEditWidget(pParent, val);
		break;
	case PropertyType::IMAGE:
		pTextureWidget_ = new AssetTextureWidget(pParent, val);
		break;
	case PropertyType::STRING:
		pStringWidget_ = new AssetStringWidget(pParent, val);
		break;
	case PropertyType::COLOR:
		pColorWidget_ = new AssetColorWidget(pParent, val);
		break;
	case PropertyType::COMBOBOX:
		pComboBoxWidget_ = new AssetComboBoxWidget(pParent, val, initData_, settings_.IsSet(Setting::EDITIABLE));
		break;
	case PropertyType::GROUPBOX:
		X_ASSERT(pGroupWidget_, "Group not valid")(pGroupWidget_);

		// pGroupWidget_ = new AssetGroupWidget(pParent);
		break;
	default:
		break;
	}

	switch (type_)
	{
	case PropertyType::CHECKBOX:
	case PropertyType::COMBOBOX:
	case PropertyType::COLOR:
	case PropertyType::STRING:
	case PropertyType::INT:
	case PropertyType::FLOAT:
	case PropertyType::PATH:
	case PropertyType::IMAGE:
	case PropertyType::LINEEDIT:
		connect(pWidget_, SIGNAL(valueChanged(const std::string&)), this, SLOT(valueChanged(const std::string&)));
		break;

	default:
		break;
	};

	// SetTile / SettoolTip
	const QString toolTip = QString::fromStdString(toolTip_);
	QString title;

	if (!title_.empty()) {
		title = QString::fromStdString(title_);
	}
	else {
		title = QString::fromStdString(key_);
	}

	if (type_ == PropertyType::CHECKBOX)
	{
		pCheckBoxWidget_->setText(title);
		pCheckBoxWidget_->setToolTip(toolTip);

		// checkbox icon hype.
		if (!icon_.isEmpty()) {
			const QString& iconPath = icon_;
			QIcon icon(iconPath);

			if (icon.pixmap(QSize(16, 16)).isNull()) {
				pCheckBoxWidget_->setIcon(QIcon(":/misc/img/warning_32.png"));

				// o baby you so sexy.
				QString missingIconTip = " (icon \"" + iconPath + "\" not fnd)";

				pCheckBoxWidget_->setToolTip(toolTip + missingIconTip);
			}
			else {
				pCheckBoxWidget_->setIcon(icon);
			}
		}

		pLayout->addWidget(pWidget_, row, depth, 1, colSpanForCol(depth));
	}
	else if (type_ == PropertyType::GROUPBOX)
	{
		pGroupWidget_->setText(title);
		pGroupWidget_->setToolTip(toolTip);

		pLayout->addWidget(pGroupWidget_, row++, depth, 1, -1);


		pGroupWidget_->appendGui(pLayout, row, depth);
	}
	else
	{	// add label.
		pLabel_ = new QLabel();
		pLabel_->setText(title);
		pLabel_->setToolTip(toolTip);

		pLayout->addWidget(pLabel_, row, depth, 1, colSpanForCol(depth));
		pLayout->addWidget(pWidget_, row, MAX_COL_IDX);
	}

	if (!settings_.IsSet(Setting::ENABLED)) {
		enable(false);
	}
	if (!settings_.IsSet(Setting::VISIBLE)) {
		show(false);
	}
}

void AssetProperty::valueChanged(const std::string& value)
{
	X_UNUSED(value);
	X_LOG0("Meow", "meow: %s", value.c_str());

	// so this prop was changed :D
	bool isModified = GetDefaultValue() != value;
	SetValue(value);
	SetModified(isModified);

	//	SetValue(value);

	emit modified();
}

int32_t AssetProperty::colSpanForCol(int32_t startCol)
{
	return MAX_COL_IDX - startCol;
}


void AssetProperty::addRef(void)
{
	++refCount_;
}

void AssetProperty::release(void)
{
	if (--refCount_ == 0) {
		delete this;
	}
}

void AssetProperty::clear(void)
{
	if (type_ == PropertyType::GROUPBOX) {
		//	pGroupWidget_->clear();
	}
}


void AssetProperty::show(bool vis)
{
	if (pLabel_) {
		pLabel_->setVisible(vis);
	}

	// how to make these simple to show hide?
	switch (type_)
	{
	case PropertyType::CHECKBOX:
		pCheckBoxWidget_->setVisible(vis);
		break;
	case PropertyType::INT:
		pSpinBoxWidget_->setVisible(vis);
		break;
	case PropertyType::FLOAT:
		pDoubleSpinBoxWidget_->setVisible(vis);
		break;
	case PropertyType::PATH:
		pPathWidget_->setVisible(vis);
		break;
	case PropertyType::TEXT:
		pTextWidget_->setVisible(vis);
		break;
	case PropertyType::IMAGE:
		pTextureWidget_->setVisible(vis);
		break;
	case PropertyType::STRING:
		pStringWidget_->setVisible(vis);
		break;
	case PropertyType::COLOR:
		pColorWidget_->setVisible(vis);
		break;
	case PropertyType::COMBOBOX:
		pComboBoxWidget_->setVisible(vis);
		break;
	case PropertyType::GROUPBOX:
		pGroupWidget_->show(vis);
		break;
	default:
		X_ASSERT_NOT_IMPLEMENTED();
		break;
	}
}


void AssetProperty::enable(bool val)
{
	if (pLabel_) {
		pLabel_->setEnabled(val);
	}

	switch (type_)
	{
	case PropertyType::CHECKBOX:
		pCheckBoxWidget_->setEnabled(val);
		break;
	case PropertyType::INT:
		pSpinBoxWidget_->setEnabled(val);
		break;
	case PropertyType::FLOAT:
		pDoubleSpinBoxWidget_->setEnabled(val);
		break;
	case PropertyType::PATH:
		pPathWidget_->setEnabled(val);
		break;
	case PropertyType::TEXT:
		pTextWidget_->setEnabled(val);
		break;
	case PropertyType::IMAGE:
		pTextureWidget_->setEnabled(val);
		break;
	case PropertyType::STRING:
		pStringWidget_->setEnabled(val);
		break;
	case PropertyType::COLOR:
		pColorWidget_->setEnabled(val);
		break;
	case PropertyType::COMBOBOX:
		pComboBoxWidget_->setEnabled(val);
		break;
	case PropertyType::GROUPBOX:
		pGroupWidget_->setEnabled(val);
		break;
	default:
		X_ASSERT_NOT_IMPLEMENTED();
		break;
	}

}

void AssetProperty::SetModified(bool modified)
{
	QString style;

	if (settings_.IsSet(Setting::MODIFIED)) {
		if (modified) {
			return;
		}

		settings_.Remove(Setting::MODIFIED);
	}
	else {
		if (!modified) {
			return;
		}

		settings_.Set(Setting::MODIFIED);
	}

	if (type_ == PropertyType::CHECKBOX)
	{
		if (modified) {
			pCheckBoxWidget_->setStyleSheet("QCheckBox { color: #a00020 }");
		}
		else {
			pCheckBoxWidget_->setStyleSheet("");
		}
	}
	else if (pLabel_)
	{
		if (modified) {
			pLabel_->setStyleSheet("QLabel { color: #a00020 }");
		}
		else {
			pLabel_->setStyleSheet("");
		}
	}
}

void AssetProperty::SetKey(const std::string& key)
{
	key_ = key;
}

void AssetProperty::SetParentKey(const std::string& key)
{
	parentKey_ = key;
}


void AssetProperty::SetType(PropertyType::Enum type)
{
	type_ = type;

	if (type_ == PropertyType::GROUPBOX) {
		X_ASSERT(!pGroupWidget_, "Group already init")(pGroupWidget_);

		pGroupWidget_ = new AssetGroupWidget();
	}
}

void AssetProperty::AddChild(AssetProperty* pChild)
{
	if (type_ == PropertyType::GROUPBOX) {
		pGroupWidget_->AddChild(pChild);
	}
	else {
		X_ASSERT_UNREACHABLE();
	}
}


AssetProperty::ConstIterator AssetProperty::begin(void) const
{
	if (type_ == PropertyType::GROUPBOX) {
		return pGroupWidget_->begin();
	}

	X_ASSERT_UNREACHABLE();
	return AssetProperty::ConstIterator();
}

AssetProperty::ConstIterator AssetProperty::end(void) const
{
	if (type_ == PropertyType::GROUPBOX) {
		return pGroupWidget_->end();
	}

	X_ASSERT_UNREACHABLE();
	return AssetProperty::ConstIterator();
}


void AssetProperty::SetTitle(const std::string& title)
{
	title_ = title;
}

void AssetProperty::SetToolTip(const std::string& toolTip)
{
	toolTip_ = toolTip;
}

void AssetProperty::SetLabels(const std::string& labelX, const std::string& labelY,
	const std::string& labelZ, const std::string& labelW)
{
	labelsX_ = QString::fromStdString(labelX);
	labelsY_ = QString::fromStdString(labelY);
	labelsZ_ = QString::fromStdString(labelZ);
	labelsW_ = QString::fromStdString(labelW);
}

void AssetProperty::SetIcon(const std::string& icon)
{
	icon_ = QString::fromStdString(icon);
}

void AssetProperty::SetFontColor(float r, float g, float b)
{
	fontCol_.setRgbF(r, g, b);
}

void AssetProperty::SetBold(bool bold)
{
	if (bold) {
		settings_.Set(Setting::BOLD_TEXT);
	}
	else {
		settings_.Remove(Setting::BOLD_TEXT);
	}
}

void AssetProperty::SetStep(double step)
{
	step_ = step;
}


void AssetProperty::SetEnabled(bool enable)
{
	if (enable) {
		settings_.Set(Setting::ENABLED);
	}
	else {
		settings_.Remove(Setting::ENABLED);
	}
}

void AssetProperty::SetVisible(bool vis)
{
	if (vis) {
		settings_.Set(Setting::VISIBLE);
	}
	else {
		settings_.Remove(Setting::VISIBLE);
	}
}

void AssetProperty::ShowJumpToAsset(bool show)
{
	if (show) {
		settings_.Set(Setting::SHOW_JUMP_TO_ASSET);
	}
	else {
		settings_.Remove(Setting::SHOW_JUMP_TO_ASSET);
	}
}

void AssetProperty::UpdateOnChange(bool update)
{
	if (update) {
		settings_.Set(Setting::UPDATE_ON_CHANGE);
	}
	else {
		settings_.Remove(Setting::UPDATE_ON_CHANGE);
	}
}

void AssetProperty::SetInitData(const std::string& val)
{
	initData_ = val;
}

void AssetProperty::SetValue(const std::string& val)
{
	strValue_ = val;
}

void AssetProperty::SetDefaultValue(const std::string& val)
{
	defaultValue_ = val;

	if (isNewProp()) {
		SetValue(val);
	}
}

void AssetProperty::SetBool(bool val)
{
	if (val) {
		strValue_ = "1";
	}
	else {
		strValue_ = "0";
	}
}

void AssetProperty::SetInt(int32_t val)
{
	strValue_ = std::to_string(val);
}

void AssetProperty::SetFloat(float val)
{
	strValue_ = std::to_string(val);
}

void AssetProperty::SetDouble(double val)
{
	strValue_ = std::to_string(val);
}


void AssetProperty::SetMinMax(int32_t min, int32_t max)
{
	min_ = static_cast<double>(min);
	max_ = static_cast<double>(max);

	// make a step
	if (min <= -100000 || max > 100000) {
		step_ = 1;
	}
	else {
		int32_t step = (max - min) / 1000;

		if (step > 1 && step < 100000) {
			step_ = static_cast<double>(step);
		}
		else {
			step_ = 1;
		}
	}
}

void AssetProperty::SetMinMax(double min, double max)
{
	min_ = min;
	max_ = max;

	if (min <= -100000 || max > 100000) {
		step_ = 1;
	}
	else {
		step_ = (max - min) / 1000;
	}
}

void AssetProperty::SetEditable(bool canEdit)
{
	if (canEdit) {
		settings_.Set(Setting::EDITIABLE);
	}
	else {
		settings_.Remove(Setting::EDITIABLE);
	}
}

void AssetProperty::SetNewProp(void)
{
	settings_.Set(Setting::NEW_PROP);
}

bool AssetProperty::isNewProp(void) const
{
	return settings_.IsSet(Setting::NEW_PROP);
}

bool AssetProperty::isModified(void) const
{
	return settings_.IsSet(Setting::MODIFIED);
}

AssetProperty::PropertyType::Enum AssetProperty::GetType(void) const
{
	return type_;
}

const std::string& AssetProperty::GetKey(void) const
{
	return key_;
}

const std::string& AssetProperty::GetParentKey(void) const
{
	return parentKey_;
}

const std::string& AssetProperty::GetTitle(void) const
{
	return title_;
}

const std::string& AssetProperty::GetToolTip(void) const
{
	return toolTip_;
}

const std::string& AssetProperty::GetValue(void) const
{
	return strValue_;
}

const std::string& AssetProperty::GetDefaultValue(void) const
{
	return defaultValue_;
}

double AssetProperty::GetValueFloat(void) const
{
	if (strValue_.empty()) {
		return 0;
	}

	return std::stod(strValue_);
}

int32_t AssetProperty::GetValueInt(void) const
{
	if (strValue_.empty()) {
		return 0;
	}

	return std::stoi(strValue_);
}

bool AssetProperty::GetValueBool(void) const
{
	return strValue_ == "1";
}


// -------------------------------------------------------------------------

AssetProperties::AssetProperties(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, AssetPropertyEditorWidget* widget) :
	IAssetEntry(widget),
	db_(db),
	pPropScriptMan_(pPropScriptMan),
	pWidget_(widget),
	pCur_(nullptr)
{
	root_.SetType(AssetProperty::PropertyType::GROUPBOX);
		
	modifiedCount_ = 0;
}

AssetProperties::~AssetProperties()
{
	clear();
}


void AssetProperties::clear(void)
{
	root_.clear();

	// kill keys?
	keys_.clear();
}


void AssetProperties::setAssetType(assetDb::AssetType::Enum type)
{
	const char* pStr = assetDb::AssetType::ToString(type);

	std::string catName(pStr);
	std::transform(catName.begin(), catName.end(), catName.begin(), ::tolower);

	if (!pCur_) {
		BeginGroup(catName);
	}
}


void AssetProperties::setWidget(QWidget* widget)
{
	if (AssetPropertyEditorWidget* wid = qobject_cast<AssetPropertyEditorWidget*>(widget))
	{
		pWidget_ = wid;
	}
}

bool AssetProperties::isModified(void) const
{
	return false;
}

bool AssetProperties::isSaveAsAllowed(void) const
{
	return false;
}

bool AssetProperties::loadProps(QString& errorString, const QString& assetName, assetDb::AssetType::Enum type)
{
	// we want to load the args from the db.
	X_UNUSED(assetName);
	auto narrowName = assetName.toLocal8Bit();
	core::string name(narrowName.data());

	int32_t assetId;
	if (!db_.AssetExsists(type, name, &assetId)) {
		errorString = "Asset `" + assetName + "` does not exsist";
		return false;
	}

	core::string args;
	if (!db_.GetArgsForAsset(assetId, args)) {
		errorString = "Failed to get asset '" + assetName + "' props";
		return false;
	}

	setAssetType(type);
	if (!parseArgs(args)) {
		errorString = "Error parsing asset '" + assetName + "' props";
		return false;
	}

	if (!pPropScriptMan_->runScriptForProps(*this, assetDb::AssetType::IMG)) {
		errorString = "Error running property script for asset '" + assetName + "'";
		return false;
	}

	if (!extractArgs(args)) {
		errorString = "Error extracting asset '" + assetName + "' props";
		return false;
	}

	QWidget* pCon = new QWidget();
	QGridLayout* pLayout = new QGridLayout();

	appendGui(pLayout);

	pCon->setLayout(pLayout);
	pWidget_->setWidget(pCon);	


	return true;
}


bool AssetProperties::parseArgs(const core::string& jsonStr)
{
	X_ASSERT(jsonStr.isNotEmpty(), "Args string should not be empty")(jsonStr.isNotEmpty());

	core::json::Document d;
	d.Parse(jsonStr.c_str());

	std::string name;

	for (auto itr = d.MemberBegin(); itr != d.MemberEnd(); ++itr)
	{
		name = itr->name.GetString();

		// add it :D
		auto& item = addItem(name);
		const auto& val = itr->value;

		switch (val.GetType())
		{
		case core::json::Type::kFalseType:
			item.SetBool(false);
			break;
		case core::json::Type::kTrueType:
			item.SetBool(true);
			break;
		case core::json::Type::kStringType:
			item.SetValue(std::string(val.GetString(), val.GetStringLength()));
			break;
		case core::json::Type::kNumberType:
			if (val.IsBool()) {
				item.SetBool(val.GetBool());
			}
			if (val.IsInt()) {
				item.SetFloat(val.GetInt());
			}
			else if (val.IsFloat()) {
				item.SetFloat(val.GetFloat());
			}
			else {
				// default to double
				item.SetDouble(val.GetDouble());
			}
			break;

			// ye fooking wut
		case core::json::Type::kObjectType:
		case core::json::Type::kArrayType:
			X_ERROR("AssetProps", "Unsupported value type for arg: %i", val.GetType());
			break;

		default:
			X_ERROR("AssetProps", "Unknown value type for arg: %i", val.GetType());
			break;
		}
	}

	return true;
}

bool AssetProperties::extractArgs(core::string& jsonStrOut) const
{
	core::json::StringBuffer s;
	core::json::Writer<core::json::StringBuffer> writer(s);

	writer.SetMaxDecimalPlaces(5);
	writer.StartObject();

	auto end = keys_.cend();
	for (auto it = keys_.cbegin(); it != end; ++it)
	{
		const auto& key = it.key();
		const auto& item = *it.value();

		writer.Key(key.c_str());
		// do we want to try work out a better type?

		switch (item.GetType())
		{
		case AssetProperty::PropertyType::BOOL:
			writer.Bool(item.GetValueBool());
			break;
		case AssetProperty::PropertyType::INT:
			writer.Int(item.GetValueInt());
			break;
		case AssetProperty::PropertyType::FLOAT:
			writer.Double(item.GetValueFloat());
			break;
		case AssetProperty::PropertyType::CHECKBOX:
			writer.Bool(item.GetValueBool());
			break;

		case AssetProperty::PropertyType::TEXT:
		case AssetProperty::PropertyType::PATH:
		case AssetProperty::PropertyType::LINEEDIT:
		case AssetProperty::PropertyType::IMAGE:
		case AssetProperty::PropertyType::COMBOBOX:
		case AssetProperty::PropertyType::COLOR:
		case AssetProperty::PropertyType::VEC2:
		case AssetProperty::PropertyType::VEC3:
		case AssetProperty::PropertyType::VEC4:
		default:
			writer.String(item.GetValue().c_str());
			break;
		}

	}

	writer.EndObject();

	jsonStrOut = core::string(s.GetString(), s.GetSize());
	return true;
}



bool AssetProperties::appendGui(QGridLayout* pLayout)
{
	for (int32_t i = 0; i < 12; i++) {
		pLayout->setColumnMinimumWidth(i, 16);
	}


	int32_t row = 0;
	int32_t depth = 0;

	for (const auto& pChild : root_)
	{
		pChild->appendGui(nullptr, pLayout, row, depth);
		row += 1;
	}

	return true;
}


void AssetProperties::BeginGroup(const std::string& groupName)
{
	std::vector<std::string> tokens;
	tokenize<std::string>(tokens, groupName, ".");

	if (tokens.empty()) {
		return;
	}

	pCur_ = &root_;

	for (const auto& token : tokens)
	{
		for (auto& pChild : *pCur_)
		{
			if (pChild->GetType() == AssetProperty::PropertyType::GROUPBOX && pChild->GetKey() == token)
			{
				// we found the group.
				pCur_ = pChild;
				goto groupExsists;
			}
		}

		// no child that's a group with curren tokens name
		// add and set as current.
		AssetProperty* pGroup = new AssetProperty();
		pGroup->SetKey(token);
		pGroup->SetTitle(token);
		pGroup->SetType(AssetProperty::PropertyType::GROUPBOX);

		// add group as child.
		pCur_->AddChild(pGroup);
		// set as current
		pCur_ = pGroup;

	groupExsists:;
	}
}


AssetProperty& AssetProperties::addItem(const std::string& key)
{
	auto it = keys_.find(key);
	if (it != keys_.end())
	{
		X_WARNING("AssetProps", "duplicate item for key: \"%s\"", key.c_str());
		return **it;
	}

	AssetProperty* pItem = new AssetProperty();
	pItem->SetKey(key);

	keys_[key] = pItem;
	return *pItem;
}

AssetProperty& AssetProperties::addItemIU(const std::string& key, AssetProperty::PropertyType::Enum type)
{
	AssetProperty* pItem = nullptr;

	auto it = keys_.find(key);
	if (it != keys_.end())
	{
		pItem = *it;

		if (pItem->GetType() == AssetProperty::PropertyType::UNCLASSIFIED)
		{
			pItem->SetType(type);

			connect(this, SIGNAL(modified(void)), pItem, SLOT(propModified(void)));
		}
		else if (pItem->GetType() != type) {
			X_WARNING("AssetProps", "Prop request with diffrent types for key \"%s\" initialType: \"%s\" requestedType: \"%s\"",
				key.c_str(), AssetProperty::PropertyType::ToString(pItem->GetType()), AssetProperty::PropertyType::ToString(type));
		}
	}
	else
	{
		// new
		pItem = new AssetProperty();
		pItem->SetKey(key);
		pItem->SetType(type);
		pItem->SetNewProp();

		// CONNECT :D !
		connect(this, SIGNAL(modified(void)), pItem, SLOT(propModified(void)));

		keys_[key] = pItem;
	}

	pCur_->AddChild(pItem);

	return *pItem;
}


AssetPropertyEditorWidget* AssetProperties::getEditor(void)
{
	return pWidget_;
}

// ----------------------------------------------------------------



AssetPropertyEditorWidget::AssetPropertyEditorWidget(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, QWidget *parent) :
	QScrollArea(parent),
	db_(db),
	pPropScriptMan_(pPropScriptMan),
	pEditor_(nullptr)
{
	assetProps_ = QSharedPointer<AssetProperties>(new AssetProperties(db, pPropScriptMan, this));

	connect(assetProps_.data(), SIGNAL(modificationChanged(bool)),
		this, SIGNAL(modificationChanged(bool)));
}

AssetPropertyEditorWidget::AssetPropertyEditorWidget(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, AssetProperties* pAssetEntry, QWidget *parent) :
	QScrollArea(parent),
	db_(db),
	pPropScriptMan_(pPropScriptMan),
	pEditor_(nullptr)
{
	assetProps_ = QSharedPointer<AssetProperties>(pAssetEntry);


	connect(assetProps_.data(), SIGNAL(modificationChanged(bool)),
		this, SIGNAL(modificationChanged(bool)));
}

AssetPropertyEditorWidget::AssetPropertyEditorWidget(AssetPropertyEditorWidget* pOther) :
	pPropScriptMan_(pOther->pPropScriptMan_),
	db_(pOther->db_),
	pEditor_(nullptr)
{
	assetProps_ = pOther->assetProps_;


	connect(assetProps_.data(), SIGNAL(modificationChanged(bool)),
		this, SIGNAL(modificationChanged(bool)));
}

AssetPropertyEditorWidget::~AssetPropertyEditorWidget()
{

}



bool AssetPropertyEditorWidget::open(QString* pErrorString, const QString& fileName, assetDb::AssetType::Enum type)
{
	X_UNUSED(pErrorString);
	X_UNUSED(fileName);
	X_UNUSED(type);

	AssetProperties* pProbs = assetProps_.data();

	// loads the props
	pProbs->setType(type);
	pProbs->setAssetName(fileName);
	if (!pProbs->loadProps(*pErrorString, fileName, type)) {
		return false;
	}

	return true;
}

AssetPropertyEditor* AssetPropertyEditorWidget::editor(void)
{
	if (!pEditor_) {
		pEditor_ = createEditor();
	}

	return pEditor_;
}

AssetProperties* AssetPropertyEditorWidget::assetProperties(void) const
{
	return assetProps_.data();
}

AssetPropertyEditor* AssetPropertyEditorWidget::createEditor(void)
{
	return new AssetPropertyEditor(this);
}

// ----------------------------------------------------------------


AssetPropertyEditor::AssetPropertyEditor(AssetPropertyEditorWidget* editor) :
	pEditorWidget_(editor)
{
	setWidget(pEditorWidget_);

	connect(editor, SIGNAL(modificationChanged(bool)), this, SLOT(modificationChanged(bool)));

}

AssetPropertyEditor::~AssetPropertyEditor()
{
	delete pEditorWidget_;
}


bool AssetPropertyEditor::open(QString* pErrorString, const QString& fileName, assetDb::AssetType::Enum type)
{
	return pEditorWidget_->open(pErrorString, fileName, type);
}

IAssetEntry* AssetPropertyEditor::assetEntry(void)
{
	return pEditorWidget_->assetProperties();
}

Id AssetPropertyEditor::id(void) const
{
	return Id(Constants::ASSETPROP_EDITOR_ID);
}

void AssetPropertyEditor::modificationChanged(bool modified)
{
	QString title = assetEntry()->displayName();

	if (modified) {
		title += "*";
	}

	emit titleChanged(title);
}

X_NAMESPACE_END