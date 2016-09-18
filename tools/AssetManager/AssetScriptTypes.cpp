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
	enabled_(true)
{

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

void AssetProperty::SetType(PropertyType type)
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
	boldText_ = bold;
	return *this;
}

AssetProperty& AssetProperty::SetStep(double step)
{
	step_ = step;
	return *this;
}


AssetProperty& AssetProperty::SetEnabled(bool enable)
{
	enabled_ = enable;
	return *this;
}

AssetProperty& AssetProperty::SetVisible(bool vis)
{
	visible_ = vis;
	return *this;
}

AssetProperty& AssetProperty::SetValue(const std::string& val)
{
	strValue_ = val;
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

bool AssetProperty::GetBool(void) const
{
	return strValue_ == "1";
}


// ----------------------------------------------------------



Asset::Asset() :
	refCount_(1)
{

}

Asset::~Asset()
{

}


void Asset::addRef(void)
{
	++refCount_;
}

void Asset::release(void)
{
	if (--refCount_ == 0) {
		delete this;
	}
}

Asset* Asset::factory(void)
{
	return new Asset();
}

Asset* Asset::copyFactory(const Asset& oth)
{
	return new Asset(oth);
}

bool Asset::createGui(QWidget* pParent)
{
	X_UNUSED(pParent);



	return true;
}

AssetProperty& Asset::AddTexture(const std::string& key, const std::string& default)
{
	AssetProperty& item = getItem(key, AssetProperty::PropertyType::IMAGE);
	item.SetValue(default);
	return item;
}

AssetProperty& Asset::AddCombo(const std::string& key, const std::string& values)
{
	AssetProperty& item = getItem(key, AssetProperty::PropertyType::COMBOBOX);
	item.SetValue(values);
	return item;
}

AssetProperty& Asset::AddCheckBox(const std::string& key, bool default)
{
	AssetProperty& item = getItem(key, AssetProperty::PropertyType::CHECKBOX);
	item.CheckBox.checked = default;
	return item;
}

AssetProperty& Asset::AddInt(const std::string& key, int32_t default, int32_t min, int32_t max)
{
	AssetProperty& item = getItem(key, AssetProperty::PropertyType::INT);
	item.Int.value = default;
	item.Int.min = min;
	item.Int.max = max;
	return item;
}

AssetProperty& Asset::AddFloat(const std::string& key, double default, double min, double max)
{
	AssetProperty& item = getItem(key, AssetProperty::PropertyType::FLOAT);
	item.Float.value = default;
	item.Float.min = min;
	item.Float.max = max;
	return item;
}

AssetProperty& Asset::AddVec2(const std::string& key, double x, double y, double min, double max)
{
	AssetProperty& item = getItem(key, AssetProperty::PropertyType::VEC2);
	item.Vec.x = x;
	item.Vec.y = y;
	item.Vec.z = 0;
	item.Vec.w = 0;
	item.Vec.min = min;
	item.Vec.max = max;
	return item;
}

AssetProperty& Asset::AddVec3(const std::string& key, double x, double y, double z, double min, double max)
{
	AssetProperty& item = getItem(key, AssetProperty::PropertyType::VEC3);
	item.Vec.x = x;
	item.Vec.y = y;
	item.Vec.z = z;
	item.Vec.w = 0;
	item.Vec.min = min;
	item.Vec.max = max;
	return item;
}

AssetProperty& Asset::AddVec4(const std::string& key, double x, double y, double z, double w, double min, double max)
{
	AssetProperty& item = getItem(key, AssetProperty::PropertyType::VEC4);
	item.Vec.x = x;
	item.Vec.y = y;
	item.Vec.z = z;
	item.Vec.w = w;
	item.Vec.min = min;
	item.Vec.max = max;
	return item;
}

AssetProperty& Asset::AddText(const std::string& key, const std::string& value)
{
	AssetProperty& item = getItem(key, AssetProperty::PropertyType::TEXT);
	item.SetValue(value);
	return item;
}

AssetProperty& Asset::AddPath(const std::string& key, const std::string& value)
{
	AssetProperty& item = getItem(key, AssetProperty::PropertyType::PATH);
	item.SetValue(value);
	return item;
}


void Asset::BeginGroup(const std::string& groupName)
{
	X_UNUSED(groupName);

}

void Asset::EndGroup(const std::string& groupName)
{
	X_UNUSED(groupName);

}

AssetProperty& Asset::getItem(const std::string& key, AssetProperty::PropertyType type)
{
	if (itemsLookup_.contains(key)) {
		return items_[itemsLookup_.value(key)];
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
