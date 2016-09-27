#include "AssetScriptTypes.h"

#include <String\Json.h>

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

AssetProperty* AssetProperty::factory(void)
{
	return new AssetProperty();
}


AssetProperty* AssetProperty::copyFactory(const AssetProperty& oth)
{
	return new AssetProperty(oth);
}


AssetProperty::AssetProperty() :
	refCount_(1),
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
	const std::string& val = defaultValue_;

	switch (type_)
	{
	case PropertyType::CHECKBOX:
		pCheckBoxWidget_ = new AssetCheckBoxWidget(pParent, val);
		break;
	case PropertyType::INT:
		pSpinBoxWidget_ = new AssetSpinBoxWidget(pParent, GetValueInt(), min_, max_, step_);
		break;
	case PropertyType::FLOAT:
		pDoubleSpinBoxWidget_ = new AssetDoubleSpinBoxWidget(pParent, GetValueFloat(), min_, max_, step_);
		break;
	case PropertyType::PATH:
		pPathWidget_ = new AssetPathWidget(pParent, val);
		break;
	case PropertyType::TEXT:
		pTextWidget_ = new AssetTextWidget(pParent, val);
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
		pComboBoxWidget_ = new AssetComboBoxWidget(pParent, val, values_, settings_.IsSet(Setting::EDITIABLE));
		break;
	case PropertyType::GROUPBOX:
		X_ASSERT(pGroupWidget_, "Group not valid")(pGroupWidget_);

		// pGroupWidget_ = new AssetGroupWidget(pParent);
		break;
	default:
		break;
	}

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
}


void AssetProperty::setLabelText(QLabel* pLabel) const
{
	if (!title_.empty()) {
		pLabel->setText(QString::fromStdString(title_));
	}
	else {
		pLabel->setText(QString::fromStdString(key_));
	}
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


AssetProperty& AssetProperty::SetTitle(const std::string& title)
{
	title_ = title;
	return *this;
}

AssetProperty& AssetProperty::SetToolTip(const std::string& toolTip)
{
	toolTip_ = toolTip;
	return *this;
}

AssetProperty& AssetProperty::SetLabels(const std::string& labelX, const std::string& labelY,
	const std::string& labelZ, const std::string& labelW)
{
	labelsX_ = QString::fromStdString(labelX);
	labelsY_ = QString::fromStdString(labelY);
	labelsZ_ = QString::fromStdString(labelZ);
	labelsW_ = QString::fromStdString(labelW);
	return *this;
}

AssetProperty& AssetProperty::SetIcon(const std::string& icon)
{
	icon_ = QString::fromStdString(icon);
	return *this;
}

AssetProperty& AssetProperty::SetFontColor(float r, float g, float b)
{
	fontCol_.setRgbF(r, g, b);
	return *this;
}

AssetProperty& AssetProperty::SetBold(bool bold)
{
	if (bold) {
		settings_.Set(Setting::BOLD_TEXT);
	}
	else {
		settings_.Remove(Setting::BOLD_TEXT);
	}
	return *this;
}

AssetProperty& AssetProperty::SetStep(double step)
{
	step_ = step;
	return *this;
}


AssetProperty& AssetProperty::SetEnabled(bool enable)
{
	if (enable) {
		settings_.Set(Setting::ENABLED);
	}
	else {
		settings_.Remove(Setting::ENABLED);
	}

	return *this;
}

AssetProperty& AssetProperty::SetVisible(bool vis)
{
	if (vis) {
		settings_.Set(Setting::VISIBLE);
	}
	else {
		settings_.Remove(Setting::VISIBLE);
	}
	return *this;
}

AssetProperty& AssetProperty::ShowJumpToAsset(bool show)
{
	if (show) {
		settings_.Set(Setting::SHOW_JUMP_TO_ASSET);
	}
	else {
		settings_.Remove(Setting::SHOW_JUMP_TO_ASSET);
	}
	return *this;
}

AssetProperty& AssetProperty::UpdateOnChange(bool update)
{
	if (update) {
		settings_.Set(Setting::UPDATE_ON_CHANGE);
	}
	else {
		settings_.Remove(Setting::UPDATE_ON_CHANGE);
	}
	return *this;
}

void AssetProperty::SetValues(const std::string& val)
{
	values_ = val;
}

AssetProperty& AssetProperty::SetValue(const std::string& val)
{
	strValue_ = val;
	return *this;
}

AssetProperty& AssetProperty::SetDefaultValue(const std::string& val)
{
	defaultValue_ = val;

	if (isNewProp()) {
		SetValue(val);
	}
	return *this;
}

AssetProperty& AssetProperty::SetBool(bool val)
{
	if (val) {
		strValue_ = "1";
	}
	else {
		strValue_ = "0";
	}

	return *this;
}

AssetProperty& AssetProperty::SetInt(int32_t val)
{
	strValue_ = std::to_string(val);
	return *this;
}

AssetProperty& AssetProperty::SetFloat(float val)
{
	strValue_ = std::to_string(val);
	return *this;
}

AssetProperty& AssetProperty::SetDouble(double val)
{
	strValue_ = std::to_string(val);
	return *this;
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

AssetProperty::PropertyType::Enum AssetProperty::GetType(void) const
{
	return type_;
}


std::string AssetProperty::GetKey(void) const
{
	return key_;
}

std::string AssetProperty::GetParentKey(void) const
{
	return parentKey_;
}

std::string AssetProperty::GetTitle(void) const
{
	return title_;
}

std::string AssetProperty::GetToolTip(void) const
{
	return toolTip_;
}

std::string AssetProperty::GetValue(void) const
{
	return strValue_;
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


// ----------------------------------------------------------


AssetProps::AssetProps() : 
	pCur_(nullptr),
	refCount_(1)
{
	root_.SetType(AssetProperty::PropertyType::GROUPBOX);

}

AssetProps::~AssetProps()
{
	clear();
}


void AssetProps::addRef(void)
{
	++refCount_;
}

void AssetProps::release(void)
{
	if (--refCount_ == 0) {
		delete this;
	}
}

void AssetProps::clear(void)
{
	root_.clear();

	// kill keys?
	keys_.clear();
}


void AssetProps::setAssetType(assetDb::AssetType::Enum type)
{
	const char* pStr = assetDb::AssetType::ToString(type);

	std::string catName(pStr);
	std::transform(catName.begin(), catName.end(), catName.begin(), ::tolower);

	if (!pCur_) {
		BeginGroup(catName);
	}
}


bool AssetProps::parseArgs(const core::string& jsonStr)
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

bool AssetProps::extractArgs(core::string& jsonStrOut) const
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


bool AssetProps::appendGui(QGridLayout* pLayout)
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

AssetProperty& AssetProps::AddTexture(const std::string& key, const std::string& default)
{
	AssetProperty& item = addItemIU(key, AssetProperty::PropertyType::IMAGE);
	item.SetDefaultValue(default);
	return item;
}

AssetProperty& AssetProps::AddCombo(const std::string& key, const std::string& valuesStr, bool editiable)
{
	AssetProperty& item = addItemIU(key, AssetProperty::PropertyType::COMBOBOX);
	item.SetEditable(editiable);

	// we pick frist from list for default.
	AssetComboBoxWidget::ComboEntryArr values;
	if (AssetComboBoxWidget::splitValues(valuesStr, values))
	{
		if (!values.isEmpty())
		{
			const auto& first = values.first();

			// do we want to set the override or the title?
			if (first.hasOverRide()) {
				item.SetDefaultValue(first.valueOverride.toStdString());
			}
			else {
				item.SetDefaultValue(first.title.toStdString());
			}
		}
	}

	item.SetValues(valuesStr);
	return item;
}

AssetProperty& AssetProps::AddCheckBox(const std::string& key, bool default)
{
	AssetProperty& item = addItemIU(key, AssetProperty::PropertyType::CHECKBOX);
	if(default) {
		item.SetDefaultValue("1");
	}
	else {
		item.SetDefaultValue("0");
	}
	return item;
}

AssetProperty& AssetProps::AddInt(const std::string& key, int32_t default, int32_t min, int32_t max)
{
	AssetProperty& item = addItemIU(key, AssetProperty::PropertyType::INT);
	item.SetDefaultValue(std::to_string(default));
	item.SetMinMax(min, max);
	return item;
}

AssetProperty& AssetProps::AddColor(const std::string& key, double r, double g, double b, double a)
{
	AssetProperty& item = addItemIU(key, AssetProperty::PropertyType::COLOR);

	QString temp = QString("%1 %2 %3 %4").arg(
		QString::number(r),
		QString::number(g),
		QString::number(b),
		QString::number(a)
	);

	item.SetDefaultValue(temp.toStdString());
	return item;
}

AssetProperty& AssetProps::AddFloat(const std::string& key, double default, double min, double max)
{
	AssetProperty& item = addItemIU(key, AssetProperty::PropertyType::FLOAT);
	item.SetDefaultValue(std::to_string(default));
	item.SetMinMax(min, max);
	return item;
}

AssetProperty& AssetProps::AddVec2(const std::string& keyX, const std::string& keyY,
	double x, double y, double min, double max)
{
	auto& itemX = AddFloat(keyX, x, min, max);
	auto& itemY = AddFloat(keyY, y, min, max);

	itemY.SetParentKey(keyX);

	return itemX;
}

AssetProperty& AssetProps::AddVec3(const std::string& keyX, const std::string& keyY, const std::string& keyZ,
	double x, double y, double z, double min, double max)
{
	auto& itemX = AddFloat(keyX, x, min, max);
	auto& itemY = AddFloat(keyY, y, min, max);
	auto& itemZ = AddFloat(keyZ, z, min, max);

	itemY.SetParentKey(keyX);
	itemZ.SetParentKey(keyX);

	return itemX;
}

AssetProperty& AssetProps::AddVec4(const std::string& keyX, const std::string& keyY, const std::string& keyZ, const std::string& keyW,
	double x, double y, double z, double w, double min, double max)
{
	auto& itemX = AddFloat(keyX, x, min, max);
	auto& itemY = AddFloat(keyY, y, min, max);
	auto& itemZ = AddFloat(keyZ, z, min, max);
	auto& itemW = AddFloat(keyW, w, min, max);

	itemY.SetParentKey(keyX);
	itemZ.SetParentKey(keyX);
	itemW.SetParentKey(keyX);

	return itemX;
}

AssetProperty& AssetProps::AddText(const std::string& key, const std::string& value)
{
	AssetProperty& item = addItemIU(key, AssetProperty::PropertyType::TEXT);
	item.SetDefaultValue(value);
	return item;
}

AssetProperty& AssetProps::AddString(const std::string& key, const std::string& value)
{
	AssetProperty& item = addItemIU(key, AssetProperty::PropertyType::STRING);
	item.SetDefaultValue(value);
	return item;
}

AssetProperty& AssetProps::AddPath(const std::string& key, const std::string& value)
{
	AssetProperty& item = addItemIU(key, AssetProperty::PropertyType::PATH);
	item.SetDefaultValue(value);
	return item;
}


void AssetProps::BeginGroup(const std::string& groupName)
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

std::string AssetProps::getPropValue(const std::string& key)
{
	return getItem(key).GetValue();
}

double AssetProps::getPropValueFloat(const std::string& key)
{
	return getItem(key).GetValueFloat();
}

int32_t AssetProps::getPropValueInt(const std::string& key)
{
	return getItem(key).GetValueInt();
}

bool AssetProps::getPropValueBool(const std::string& key)
{
	return getItem(key).GetValueBool();
}

AssetProperty& AssetProps::getItem(const std::string& key)
{
	auto it = keys_.find(key);
	if (it != keys_.end()) {
		return **it;
	}

	X_ERROR("AssetProps", "Requested undefined entry \"%s\"", key.c_str());

	// umm what todo, since we can't add it as we don't know the type.
	// just return empty object? 
	static AssetProperty empty;
	empty.SetValue("");
	return empty;
}


AssetProperty& AssetProps::addItem(const std::string& key)
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

AssetProperty& AssetProps::addItemIU(const std::string& key, AssetProperty::PropertyType::Enum type)
{
	AssetProperty* pItem = nullptr;


	auto it = keys_.find(key);
	if (it != keys_.end())
	{
		pItem = *it;
		
		if (pItem->GetType() == AssetProperty::PropertyType::UNCLASSIFIED)
		{
			pItem->SetType(type);
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
		keys_[key] = pItem;
	}

	pCur_->AddChild(pItem);

	return *pItem;
}

// -----------------------------------------------------------

AssetProps* AssetProps::factory(void)
{
	return new AssetProps();
}

AssetProps* AssetProps::copyFactory(const AssetProps& oth)
{
	return new AssetProps(oth);
}




X_NAMESPACE_END
