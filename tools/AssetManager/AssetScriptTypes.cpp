#include "AssetScriptTypes.h"

#include <String\Json.h>

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
	step_(0.0001)
{
	settings_.Set(Setting::VISIBLE);
	settings_.Set(Setting::ENABLED);
}

AssetProperty::~AssetProperty()
{
	clear();
}

void AssetProperty::appendGui(QGridLayout* pLayout, int32_t& row, int32_t depth)
{
	switch (type_)
	{
	case PropertyType::CHECKBOX:
		asCheckBox(pLayout, row, depth);
		return;
	case PropertyType::TEXT:
		asText(pLayout, row, depth);
		return;
	case PropertyType::INT:
		asIntSpin(pLayout, row, depth);
		return;
	case PropertyType::FLOAT:
		asFloatSpin(pLayout, row, depth);
		return;
	case PropertyType::COLOR:
		asColor(pLayout, row, depth);
		return;
	case PropertyType::GROUPBOX:
		asGroupBox(pLayout, row, depth);
		return;
	default:
		break;
	}

	// add label.
	QLabel* pLabel = new QLabel();
	setLabelText(pLabel);

	QLabel* pNoTypeLabel = new QLabel();
	pNoTypeLabel->setText(QString("No widget for type: ") + PropertyType::ToString(type_));
	pNoTypeLabel->setStyleSheet("QLabel { color : red; }");

	auto font = pNoTypeLabel->font();
	font.setBold(true);
	pNoTypeLabel->setFont(font);

	pLayout->addWidget(pLabel, row, depth, 1, colSpanForCol(depth));
	pLayout->addWidget(pNoTypeLabel, row, MAX_COL_IDX);
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

void AssetProperty::asGroupBox(QGridLayout* pLayout, int32_t& row, int32_t depth)
{
	QToolButton* pExpandButton = new QToolButton();
	pExpandButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	pExpandButton->setAutoRaise(true);
	pExpandButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
	pExpandButton->setText(QString::fromStdString(title_));

	auto font = pExpandButton->font();
	font.setBold(true);
	pExpandButton->setFont(font);

	// font.setPixelSize(;)
	{
		pExpandButton->setIcon(QIcon(":/misc/img/collapse.png"));

		pExpandButton->setIconSize(QSize(12, 12));
	}


	pLayout->addWidget(pExpandButton, row++, depth, 1, -1);

	// add all the items.
	for (auto& pChild : children_)
	{
		pChild->appendGui(pLayout, row, depth + 1);
		row += 1;
	}
}



void AssetProperty::asCheckBox(QGridLayout* pLayout, int32_t row,  int32_t depth)
{
	QCheckBox* pCheckBox = new QCheckBox();
	if (!title_.empty()) {
		pCheckBox->setText(QString::fromStdString(title_));
	}
	else {
		pCheckBox->setText(QString::fromStdString(key_));
	}

	if (!icon_.empty()) {
		QIcon icon(QString::fromStdString(icon_));
		pCheckBox->setIcon(icon);
	}

	pLayout->addWidget(pCheckBox, row, depth, 1, -1); // -1 spans to far right col.
}


void AssetProperty::asText(QGridLayout* pLayout, int32_t row, int32_t depth)
{
	QLineEdit* pLineEdit = new QLineEdit();
	if (!strValue_.empty()) {
		pLineEdit->setText(QString::fromStdString(strValue_));
	}
	else if(!defaultValue_.empty()) {
		pLineEdit->setText(QString::fromStdString(defaultValue_));
	}

	QLabel* pLabel = new QLabel();
	setLabelText(pLabel);

	pLayout->addWidget(pLabel, row, depth, 1, colSpanForCol(depth));
	pLayout->addWidget(pLineEdit, row, MAX_COL_IDX);
}

void AssetProperty::asIntSpin(QGridLayout* pLayout, int32_t row, int32_t depth)
{
	QSpinBox* pSpin = new QSpinBox();
	pSpin->setValue(GetValueInt());
	pSpin->setRange(static_cast<int32_t>(min_), static_cast<int32_t>(max_));
	pSpin->setSingleStep(static_cast<int32_t>(step_));

	QLabel* pLabel = new QLabel();
	setLabelText(pLabel);

	pLayout->addWidget(pLabel, row, depth, 1, colSpanForCol(depth));
	pLayout->addWidget(pSpin, row, MAX_COL_IDX);
}

void AssetProperty::asFloatSpin(QGridLayout* pLayout, int32_t row, int32_t depth)
{
	QDoubleSpinBox* pSpin = new QDoubleSpinBox();
	pSpin->setValue(GetValueInt());
	pSpin->setRange(min_, max_);
	pSpin->setSingleStep(step_);

	QLabel* pLabel = new QLabel();
	setLabelText(pLabel);

	pLayout->addWidget(pLabel, row, depth, 1, colSpanForCol(depth));
	pLayout->addWidget(pSpin, row, MAX_COL_IDX);
}


void AssetProperty::asColor(QGridLayout* pLayout, int32_t row, int32_t depth)
{
	QHBoxLayout* pChildLayout = new QHBoxLayout();
	pChildLayout->setContentsMargins(0, 0, 0, 0);
	{
		QLabel* pColLabel = new QLabel();
		pColLabel->setFrameStyle(QFrame::Box | QFrame::Panel | QFrame::Plain | QFrame::Raised);
		pColLabel->setAlignment(Qt::AlignCenter);
		pColLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		pColLabel->setMinimumWidth(64);

		pChildLayout->addWidget(pColLabel);

		QToolButton* pButton = new QToolButton();
		pButton->setToolTip("Color Picker");
		pButton->setIcon(QIcon(":/misc/img/colorpicker.png"));

		pChildLayout->addWidget(pButton);

		const char* pLabels[4] = { "R", "G", "B", "A" };

		for (int32_t i = 0; i < 4; i++)
		{
			QLabel* pLabel = new QLabel();
			pLabel->setText(pLabels[i]);

			QLineEdit* pLineEdit = new QLineEdit();
			pLineEdit->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
			pLineEdit->setMaximumWidth(64);
			pLineEdit->setValidator(new QIntValidator(0, 255));

			pChildLayout->addWidget(pLabel, 0);
			pChildLayout->addWidget(pLineEdit, 0);
		}
	}

	QLabel* pLabel = new QLabel();
	setLabelText(pLabel);

	pLayout->addWidget(pLabel, row, depth, 1, colSpanForCol(depth));
	pLayout->addLayout(pChildLayout, row, MAX_COL_IDX);
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
	for (auto pChild : children_)
	{
		delete pChild;
	}
	
	children_.clear();
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
}

void AssetProperty::AddChild(AssetProperty* pChild)
{
	children_.push_back(pChild);
}


AssetProperty::ConstIterator AssetProperty::begin(void) const
{
	return children_.cbegin();
}

AssetProperty::ConstIterator AssetProperty::end(void) const
{
	return children_.cend();
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
	labelsX_ = labelX;
	labelsY_ = labelY;
	labelsZ_ = labelZ;
	labelsW_ = labelW;
	return *this;
}

AssetProperty& AssetProperty::SetIcon(const std::string& icon)
{
	icon_ = icon;
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


AssetProperty& AssetProperty::SetValue(const std::string& val)
{
	strValue_ = val;
	return *this;
}

AssetProperty& AssetProperty::SetDefaultValue(const std::string& val)
{
	defaultValue_ = val;
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
	refCount_(1),
	groupDepth_(1)
{
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

bool AssetProps::parseArgs(const std::string& jsonStr)
{
	core::json::Document d;
	d.Parse(jsonStr.c_str());

	std::string name;

	for (auto itr = d.MemberBegin(); itr != d.MemberEnd(); ++itr)
	{
		name = itr->name.GetString();

		// add it :D
		auto& item = addItem(name, AssetProperty::PropertyType::UNCLASSIFIED);
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

bool AssetProps::extractArgs(std::string& jsonStrOut) const
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
		case AssetProperty::PropertyType::IMAGE:
		case AssetProperty::PropertyType::PATH:
		case AssetProperty::PropertyType::COMBOBOX:
		default:
			writer.String(item.GetValue().c_str());
			break;
		}

	}

	writer.EndObject();

	jsonStrOut = s.GetString();
	return true;
}


bool AssetProps::appendGui(QGridLayout* pLayout)
{
	for (int32_t i = 0; i < groupDepth_; i++) {
		pLayout->setColumnMinimumWidth(i, 16);
	}

	// hellow my little goat.
	// what to give the children!
	// candy? or 50 lashes!?
	int32_t row = 0;
	int32_t depth = 0;

	for (const auto& pChild : root_)
	{
		pChild->appendGui(pLayout, row, depth);
		row += 1;
	}

	return true;
}

AssetProperty& AssetProps::AddTexture(const std::string& key, const std::string& default)
{
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::IMAGE);
	item.SetDefaultValue(default);
	return item;
}

AssetProperty& AssetProps::AddCombo(const std::string& key, const std::string& values)
{
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::COMBOBOX);
	item.SetDefaultValue(values);
	return item;
}

AssetProperty& AssetProps::AddCheckBox(const std::string& key, bool default)
{
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::CHECKBOX);
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
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::INT);
	item.SetDefaultValue(std::to_string(default));
	item.SetMinMax(min, max);
	return item;
}

AssetProperty& AssetProps::AddFloat(const std::string& key, double default, double min, double max)
{
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::FLOAT);
	item.SetDefaultValue(std::to_string(default));
	item.SetMinMax(min, max);
	return item;
}

AssetProperty& AssetProps::AddColor(const std::string& key, double r, double g, double b, double a)
{
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::COLOR);

	QString temp = QString("%1 %2 %3 %4").arg(
		QString::number(r),
		QString::number(g),
		QString::number(b),
		QString::number(a)
	);

	item.SetDefaultValue(temp.toStdString());
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
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::TEXT);
	item.SetDefaultValue(value);
	return item;
}

AssetProperty& AssetProps::AddPath(const std::string& key, const std::string& value)
{
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::PATH);
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

	groupDepth_ = core::Max(groupDepth_, core::Max(1, static_cast<int32_t>(tokens.size()))) + 1;

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


AssetProperty& AssetProps::addItem(const std::string& key, AssetProperty::PropertyType::Enum type)
{
	auto it = keys_.find(key);
	if (it != keys_.end()) 
	{
		auto& pItem = *it;

		if (pItem->GetType() == AssetProperty::PropertyType::UNCLASSIFIED)
		{
			pItem->SetType(type);
		}
		else if (pItem->GetType() != type) {
			X_WARNING("AssetProps", "Prop request with diffrent types for key \"%s\" initialType: \"%s\" requestedType: \"%s\"",
				key.c_str(), AssetProperty::PropertyType::ToString(pItem->GetType()), AssetProperty::PropertyType::ToString(type));
		}
		return *pItem;
	}

	AssetProperty* pItem = new AssetProperty();
	pItem->SetKey(key);
	pItem->SetType(type);

	if (!pCur_) {
		BeginGroup("xmodel");
	}

	pCur_->AddChild(pItem);

	keys_[key] = pItem;
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
