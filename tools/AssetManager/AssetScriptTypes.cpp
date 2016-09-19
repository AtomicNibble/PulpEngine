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
	return std::stoi(strValue_);
}

bool AssetProperty::GetValueBool(void) const
{
	return strValue_ == "1";
}


// ----------------------------------------------------------


AssetProps::AssetProps() : 
	pCur_(&root_),
	refCount_(1)
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
		const auto& val = it.value()->GetValue();

		writer.Key(key.c_str());
		// do we want to try work out a better type?
		writer.String(val.c_str());
	}

	writer.EndObject();

	jsonStrOut = s.GetString();
	return true;
}


bool AssetProps::createGui(QWidget* pParent)
{
	X_UNUSED(pParent);



	return true;
}

AssetProperty& AssetProps::AddTexture(const std::string& key, const std::string& default)
{
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::IMAGE);
	item.SetValue(default);
	return item;
}

AssetProperty& AssetProps::AddCombo(const std::string& key, const std::string& values)
{
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::COMBOBOX);
	item.SetValue(values);
	return item;
}

AssetProperty& AssetProps::AddCheckBox(const std::string& key, bool default)
{
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::CHECKBOX);
	item.SetBool(default);
	return item;
}

AssetProperty& AssetProps::AddInt(const std::string& key, int32_t default, int32_t min, int32_t max)
{
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::INT);
	item.SetInt(default);
	item.SetMinMax(min, max);
	return item;
}

AssetProperty& AssetProps::AddFloat(const std::string& key, double default, double min, double max)
{
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::FLOAT);
	item.SetDouble(default); // it's double but yeh.. fuck you!
	item.SetMinMax(min, max);
	return item;
}

AssetProperty& AssetProps::AddVec2(const std::string& keyX, const std::string& keyY,
	double x, double y, double min, double max)
{
	AssetProperty& item = addItem(keyX, AssetProperty::PropertyType::VEC2);
	X_UNUSED(keyY);
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(min);
	X_UNUSED(max);

	return item;
}

AssetProperty& AssetProps::AddVec3(const std::string& keyX, const std::string& keyY, const std::string& keyZ,
	double x, double y, double z, double min, double max)
{
	AssetProperty& item = addItem(keyX, AssetProperty::PropertyType::VEC2);
	X_UNUSED(keyY);
	X_UNUSED(keyZ);
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(z);
	X_UNUSED(min);
	X_UNUSED(max);

	return item;
}

AssetProperty& AssetProps::AddVec4(const std::string& keyX, const std::string& keyY, const std::string& keyZ, const std::string& keyW,
	double x, double y, double z, double w, double min, double max)
{
	AssetProperty& item = addItem(keyX, AssetProperty::PropertyType::VEC2);
	X_UNUSED(keyY);
	X_UNUSED(keyZ);
	X_UNUSED(keyW);
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(z);
	X_UNUSED(w);
	X_UNUSED(min);
	X_UNUSED(max);


	// we give each item it's own key.
	// just need to decide how best to organise it so the gui ends up grouped.

	return item;
}

AssetProperty& AssetProps::AddText(const std::string& key, const std::string& value)
{
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::TEXT);
	item.SetValue(value);
	return item;
}

AssetProperty& AssetProps::AddPath(const std::string& key, const std::string& value)
{
	AssetProperty& item = addItem(key, AssetProperty::PropertyType::PATH);
	item.SetValue(value);
	return item;
}


void AssetProps::BeginGroup(const std::string& groupName)
{
	std::vector<std::string> tokens;	
	tokenize<std::string>(tokens, groupName, ".");

	if (tokens.empty()) {
		return;
	}

	if (!pCur_) {
		root_.SetKey(tokens[0]);
		root_.SetType(AssetProperty::PropertyType::GROUPBOX);
		pCur_ = &root_;
	}

	for (const auto& token : tokens)
	{
		for (auto& pChild : *pCur_)
		{
			if (pChild->GetType() == AssetProperty::PropertyType::GROUPBOX && pChild->GetKey() == tokens[0])
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
