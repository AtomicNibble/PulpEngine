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

AssetProperty& AssetProperty::operator=(const AssetProperty& oth)
{
	enabled_ = oth.enabled_;
	title_ = oth.title_;
	return *this;
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

AssetProperty& AssetProperty::SetEnabled(bool enable)
{
	enabled_ = enable;
	return *this;
}

AssetProperty& AssetProperty::SetVisible(bool enable)
{
	X_UNUSED(enable);

	return *this;
}



// ----------------------------------------------------------



Asset::Asset() :
	refCount_(1)
{

}

Asset::~Asset()
{

}

Asset& Asset::operator=(const Asset& oth)
{
	items_ = oth.items_;
	refCount_ = oth.refCount_;
	return *this;
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

AssetProperty& Asset::AddTexture(const std::string& key, const std::string& default)
{
	AssetProperty& item = getItem(key, AssetProperty::PropertyType::IMAGE);
	item.strValue_ = default;

	return item;
}

AssetProperty& Asset::AddCombo(const std::string& key, const std::string& values)
{
	AssetProperty& item = getItem(key, AssetProperty::PropertyType::COMBOBOX);
	item.strValue_ = values;

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

AssetProperty& Asset::AddFloat(const std::string& key, float default, float min, float max)
{
	AssetProperty& item = getItem(key, AssetProperty::PropertyType::FLOAT);
	item.Float.value = default;
	item.Float.min = min;
	item.Float.max = max;

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
	AssetProperty& item = items_[key];
	item.SetType(type);
	return item;
}





X_NAMESPACE_END
