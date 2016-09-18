#include "AssetScriptTypes.h"


X_NAMESPACE_BEGIN(assman)

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

void AssetProperty::SetPropertyName(const std::string& property)
{
	property_ = property;
}

void AssetProperty::SetType(PropertyType::Enum type)
{
	type_ = type;
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
	refCount_(1)
{

}

AssetProps::~AssetProps()
{

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

AssetProps* AssetProps::factory(void)
{
	return new AssetProps();
}

AssetProps* AssetProps::copyFactory(const AssetProps& oth)
{
	return new AssetProps(oth);
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
	X_UNUSED(groupName);

}

void AssetProps::EndGroup(const std::string& groupName)
{
	X_UNUSED(groupName);

}

AssetProperty& AssetProps::getItem(const std::string& key)
{
	if (itemsLookup_.contains(key)) {
		auto& item = items_[itemsLookup_.value(key)];;
	
		return item;
	}

	static AssetProperty notFnd;
	return notFnd;
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

AssetProperty& AssetProps::addItem(const std::string& key, AssetProperty::PropertyType::Enum type)
{
	if (itemsLookup_.contains(key)) {
		auto& item = items_[itemsLookup_.value(key)];;
		if (item.GetType() != type) {
			X_WARNING("AssetProps", "Prop request with diffrent types for key \"%s\" initialType: \"%s\" requestedType: \"%s\"",
				key.c_str(), AssetProperty::PropertyType::ToString(item.GetType()), AssetProperty::PropertyType::ToString(type));
		}
		return item;
	}

	items_.push_back(AssetProperty());
	const int32_t idx = items_.size() - 1;

	itemsLookup_[key] = idx;

	AssetProperty& item = items_[idx];
	item.SetPropertyName(key);
	item.SetType(type);
	return item;
}





X_NAMESPACE_END
