#include "AssetScriptTypes.h"
#include "AssetPropertyEditor.h"

#include "AssetComboBoxWidget.h"

#include <IMaterial.h>

#include <../../tools/MaterialLib/MatLib.h>

X_NAMESPACE_BEGIN(assman)


// -----------------------------------------------------------

AssetScriptProperty::AssetScriptProperty() :
	pProp_(nullptr),
	refCount_(1)
{
	pProp_ = new AssetProperty();
	pProp_->SetType(AssetProperty::PropertyType::UNCLASSIFIED);
}

AssetScriptProperty::AssetScriptProperty(AssetProperty* pProp) :
	pProp_(pProp),
	refCount_(1)
{
	pProp->addRef();
}

AssetScriptProperty::AssetScriptProperty(const AssetScriptProperty& oth) 
{
	pProp_ = oth.pProp_;
	refCount_ = oth.refCount_;
}

AssetScriptProperty::~AssetScriptProperty()
{
	if (pProp_) {
		pProp_->release();
	}

#if X_DEBUG
	pProp_ = nullptr;
	refCount_ = 0;
#endif // !X_DEBUG
}

AssetScriptProperty& AssetScriptProperty::operator=(const AssetScriptProperty& oth)
{
	pProp_ = oth.pProp_;
	refCount_ = oth.refCount_;
	return *this;
}

AssetProperty& AssetScriptProperty::prop(void)
{
	return *pProp_;
}

void AssetScriptProperty::addRef(void)
{
	++refCount_;
}

void AssetScriptProperty::release(void)
{
	if (--refCount_ == 0) {
		delete this;
	}
}


AssetScriptProperty* AssetScriptProperty::SetTitle(const std::string& title)
{
	pProp_->SetTitle(title);

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::SetToolTip(const std::string& toolTip)
{
	pProp_->SetToolTip(toolTip);

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::SetLabels(const std::string& labelX, const std::string& labelY,
	const std::string& labelZ, const std::string& labelW)
{
	pProp_->SetLabels(labelX, labelY, labelZ, labelW);

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::SetIcon(const std::string& icon)
{
	pProp_->SetIcon(icon);

	addRef();
	return this;
}

AssetScriptProperty*AssetScriptProperty::SetFontColor(float r, float g, float b)
{
	pProp_->SetFontColor(r,g,b);

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::SetBold(bool bold)
{
	pProp_->SetBold(bold);

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::SetStep(double step)
{
	pProp_->SetStep(step);

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::SetEnabled(bool enable)
{
	pProp_->SetEnabled(enable);

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::SetVisible(bool vis)
{
	pProp_->SetVisible(vis);

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::ShowJumpToAsset(bool show)
{
	pProp_->ShowJumpToAsset(show);

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::UpdateOnChange(bool update)
{
	pProp_->UpdateOnChange(update);

	addRef();
	return this;
}


AssetScriptProperty* AssetScriptProperty::SetValue(const std::string& val)
{
	pProp_->SetValue(val);

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::SetDefaultValue(const std::string& val)
{
	pProp_->SetDefaultValue(val);

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::SetBool(bool val)
{
	pProp_->SetValue(val ? "1" : "0");

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::SetInt(int32_t val)
{
	pProp_->SetValue(std::to_string(val));

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::SetFloat(float val)
{
	pProp_->SetValue(std::to_string(val));

	addRef();
	return this;
}

AssetScriptProperty* AssetScriptProperty::SetDouble(double val)
{
	pProp_->SetValue(std::to_string(val));

	addRef();
	return this;
}


std::string AssetScriptProperty::GetTitle(void) const
{
	return pProp_->GetTitle();
}

std::string AssetScriptProperty::GetToolTip(void) const
{
	return pProp_->GetToolTip();
}

std::string AssetScriptProperty::GetValue(void) const
{
	return pProp_->GetValue();
}

double AssetScriptProperty::GetValueFloat(void) const
{
	return pProp_->GetValueFloat();
}

int32_t AssetScriptProperty::GetValueInt(void) const
{
	return pProp_->GetValueBool();
}

bool AssetScriptProperty::GetValueBool(void) const
{
	return pProp_->GetValueBool();
}



AssetScriptProperty* AssetScriptProperty::factory(void)
{
	return new AssetScriptProperty();
}

AssetScriptProperty* AssetScriptProperty::copyFactory(const AssetScriptProperty& oth)
{
	return new AssetScriptProperty(oth);
}


// -----------------------------------------------------------


AssetScriptProps::AssetScriptProps(AssetProperties& props, engine::techset::TechSetDefs& techDefs) :
	props_(props),
	techDefs_(techDefs),
	refCount_(1)
{

}

AssetScriptProps::~AssetScriptProps()
{

}

void AssetScriptProps::addRef(void)
{

}

void AssetScriptProps::release(void)
{

}

AssetScriptProperty* AssetScriptProps::AddVideo(const std::string& key, const std::string& default)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::VIDEO);
	pProp->prop().SetDefaultValue(default);
	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddFont(const std::string& key, const std::string& default)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::FONT);
	pProp->prop().SetDefaultValue(default);
	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddTexture(const std::string& key, const std::string& default)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::IMAGE);
	pProp->prop().SetDefaultValue(default);
	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddModel(const std::string& key, const std::string& default)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::MODEL);
	pProp->prop().SetDefaultValue(default);
	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddAnim(const std::string& key, const std::string& default)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::ANIM);
	pProp->prop().SetDefaultValue(default);
	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddAssetRef(const std::string& key, const std::string& type)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::ASSET_REF);

	bool newProp = pProp->prop().isNewProp();

	pProp->prop().SetDefaultValue(type);
	if (newProp) {
		pProp->prop().SetSavedValue("");
	}

	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddCombo(const std::string& key, const std::string& valuesStr, bool editiable)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::COMBOBOX);
	pProp->prop().SetEditable(editiable);

	// we pick frist from list for default.
	AssetComboBoxWidget::ComboEntryArr values;
	if (AssetComboBoxWidget::splitValues(valuesStr, values))
	{
		if (!values.isEmpty())
		{
			const auto& first = values.first();

			pProp->prop().SetDefaultValue(first.value.toStdString());
		}
	}

	pProp->prop().SetInitData(valuesStr);
	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddCheckBox(const std::string& key, bool default)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::CHECKBOX);
	if (default) {
		pProp->prop().SetDefaultValue("1");
	}
	else {
		pProp->prop().SetDefaultValue("0");
	}
	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddInt(const std::string& key, int32_t default, int32_t min, int32_t max)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::INT);
	pProp->prop().SetDefaultValue(std::to_string(default));
	pProp->prop().SetMinMax(min, max);
	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddColor(const std::string& key, double r, double g, double b, double a)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::COLOR);

	QString temp = QString("%1 %2 %3 %4").arg(
		QString::number(r),
		QString::number(g),
		QString::number(b),
		QString::number(a)
	);

	pProp->prop().SetDefaultValue(temp.toStdString());
	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddFloat(const std::string& key, double default, double min, double max)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::FLOAT);
	pProp->prop().SetDefaultValue(std::to_string(default));
	pProp->prop().SetMinMax(min, max);
	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddVec2(const std::string& keyX, const std::string& keyY,
	double x, double y, double min, double max)
{
	auto itemX = AddFloat(keyX, x, min, max);
	auto itemY = AddFloat(keyY, y, min, max);

	itemY->prop().SetParentKey(keyX);

	return itemX;
}

AssetScriptProperty* AssetScriptProps::AddVec3(const std::string& keyX, const std::string& keyY, const std::string& keyZ,
	double x, double y, double z, double min, double max)
{
	auto itemX = AddFloat(keyX, x, min, max);
	auto itemY = AddFloat(keyY, y, min, max);
	auto itemZ = AddFloat(keyZ, z, min, max);

	itemY->prop().SetParentKey(keyX);
	itemZ->prop().SetParentKey(keyX);

	return itemX;
}

AssetScriptProperty* AssetScriptProps::AddVec4(const std::string& keyX, const std::string& keyY, const std::string& keyZ, const std::string& keyW,
	double x, double y, double z, double w, double min, double max)
{
	auto itemX = AddFloat(keyX, x, min, max);
	auto itemY = AddFloat(keyY, y, min, max);
	auto itemZ = AddFloat(keyZ, z, min, max);
	auto itemW = AddFloat(keyW, w, min, max);

	itemY->prop().SetParentKey(keyX);
	itemZ->prop().SetParentKey(keyX);
	itemW->prop().SetParentKey(keyX);

	return itemX;
}

AssetScriptProperty* AssetScriptProps::AddText(const std::string& key, const std::string& value)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::IMAGE);
	pProp->prop().SetDefaultValue(value);
	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddString(const std::string& key, const std::string& value)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::STRING);
	pProp->prop().SetDefaultValue(value);
	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddPath(const std::string& key, const std::string& value)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::PATH);
	pProp->prop().SetDefaultValue(value);
	return pProp;
}

AssetScriptProperty* AssetScriptProps::AddLabel(const std::string& key, const std::string& value)
{
	auto pProp = getProperty(key, AssetProperty::PropertyType::LABEL);
	pProp->prop().SetDefaultValue(value);
	return pProp;
}

void AssetScriptProps::BeginGroup(const std::string& groupName)
{
	props_.BeginGroup(groupName);
}


AssetScriptProperty* AssetScriptProps::getItem(const std::string& key)
{
	// we return a new unclaffified item if one not fnd, so we always return valid.
	return getProperty(key, AssetProperty::PropertyType::UNCLASSIFIED);
}

std::string AssetScriptProps::getPropValue(const std::string& key)
{
	return getItem(key)->GetValue();
}

double AssetScriptProps::getPropValueFloat(const std::string& key)
{
	return getItem(key)->GetValueFloat();
}

int32_t AssetScriptProps::getPropValueInt(const std::string& key)
{
	return getItem(key)->GetValueInt();
}

bool AssetScriptProps::getPropValueBool(const std::string& key)
{
	return getItem(key)->GetValueBool();
}

void AssetScriptProps::showProp(const std::string& key)
{
	// so this shows the prop, but it also allows a prop to shif cat.
	// so the cat this function is called in is where the prop ends up.
	// basically allowing a prop to be re 'cat'

	auto it = map_.find(key);
	if (it != map_.end())
	{
		auto* pProp = *it;
		pProp->prop().SetVisible(true);

		// move it's cat.
		props_.showInCurrentCat(pProp->prop());
	}
	else
	{
		X_WARNING("AssetScript", "Failed to find prop to show: \"%s\"", key.c_str());
	}
}

std::string AssetScriptProps::getMaterialCats(void)
{
	std::string s;
	core::StackString<128, char> temp;

	s.reserve(engine::MaterialCat::ENUM_COUNT * 16);

	for (uint32_t i = 0; i < engine::MaterialCat::ENUM_COUNT; i++)
	{
		engine::MaterialCat::Enum type = static_cast<engine::MaterialCat::Enum>(i);
		if (type == engine::MaterialCat::UNKNOWN) {
			continue;
		}

		const char* pName = engine::MaterialCat::ToString(i);
		temp.set(pName);
		temp.toLower();

		s += temp.c_str();
		s += "|";
	}

	return s;
}


std::string AssetScriptProps::getMaterialTypes(std::string& catStr)
{
	if (catStr.empty()) {
		return "";
	}

	const engine::MaterialCat::Enum cat = engine::Util::MatCatFromStr(catStr.data(), catStr.data() + catStr.length());

	// we need to load the tech defs for this cat :|
	engine::techset::TechSetDefs::CatTypeArr types(g_arena);
	if (!engine::techset::TechSetDefs::getTechCatTypes(cat, types)) {
		X_ERROR("AssetScript", "Failed to get tech cat type");
		return "<error>";
	}

	if (types.isEmpty()) {
		return "<none>";
	}

	std::string s;
	for (const auto& t : types)
	{
		s += t;
		s += "|";
	}

	return s;
}

bool AssetScriptProps::isMaterialType(std::string& catStr, std::string& typeStr)
{
	const engine::MaterialCat::Enum cat = engine::Util::MatCatFromStr(catStr.data(), catStr.data() + catStr.length());

	if (cat == engine::MaterialCat::UNKNOWN) {
		return false;
	}

	engine::techset::TechSetDefs::CatTypeArr types(g_arena);
	if (!techDefs_.getTechCatTypes(cat, types)) {
		return false;
	}

	for (const auto& t : types)
	{
		if (t.compare(typeStr.c_str()))
		{
			return true;
		}
	}

	return false;
}

void AssetScriptProps::addMaterialTypeProps(std::string& catStr, std::string& typeStr)
{
	const engine::MaterialCat::Enum cat = engine::Util::MatCatFromStr(catStr.data(), catStr.data() + catStr.length());

	// we wnat the TECH DEF!
	// then once we have it we get all the params
	// we just look for them and enable.
	auto* pTechDef = techDefs_.getTechDef(cat, typeStr.c_str());
	if(!pTechDef) {
		return;
	}

	// right now we just need to show the props that are part of this techSetDef.

	auto showProps = [&](const core::string& propName, const engine::techset::AssManProps& assProps) {
		auto propIt = map_.find(std::string(propName));
		if (propIt != map_.end())
		{
			auto* pProp = *(propIt);

			pProp->prop().SetVisible(true);

			// set the active cat.
			if (assProps.cat.isNotEmpty())
			{
				props_.BeginGroup(std::string(assProps.cat));
				props_.showInCurrentCat(pProp->prop());
			}

			if (assProps.title.isNotEmpty())
			{
				pProp->SetTitle(std::string(assProps.title));
			}
			if (assProps.defaultVal.isNotEmpty())
			{
				pProp->SetDefaultValue(std::string(assProps.defaultVal));
			}
		}
		else
		{
			X_WARNING("AssetScript", "Failed to find techSet prop: \"%s\"", propName.c_str());
		}
	};

	for (auto& it : pTechDef->getParams())
	{
		const auto& propName = it.first;
		const auto& param = it.second;

		showProps(propName, param.assProps);
	}

	for (auto& it : pTechDef->getTextures())
	{
		// const auto& texName = it->first;
		const auto& tex = it.second;

		showProps(tex.propName, tex.assProps);
	}


	for (auto& it : pTechDef->getSamplers())
	{
	//	const auto& samplerName = it.first;
		const auto& samplerDesc = it.second;

		if (!samplerDesc.isFilterDefined())
		{
			showProps(samplerDesc.filterStr, samplerDesc.assProps);
		}

		if (!samplerDesc.isRepeateDefined())
		{
			showProps(samplerDesc.repeatStr, samplerDesc.assProps);
		}
	}
}

AssetScriptProperty* AssetScriptProps::getProperty(const std::string& key, AssetProperty::PropertyType::Enum type)
{
	AssetScriptProperty* pScriptProp = nullptr;

	auto it = map_.find(key);
	if (it == map_.end()) 
	{		
		// we don't have a property item for it :/
		auto& prop = props_.addItemIU(key, type);
		pScriptProp = new AssetScriptProperty(&prop);

		map_[key] = pScriptProp;
	}
	else 
	{
		pScriptProp = *it;
	}

	pScriptProp->addRef();
	return pScriptProp;
}


X_NAMESPACE_END
