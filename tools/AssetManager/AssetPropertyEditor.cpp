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
#include "AssetVideoWidget.h"
#include "AssetFontWidget.h"
#include "AssetTextureWidget.h"
#include "AssetModelWidget.h"
#include "AssetAnimWidget.h"
#include "AssetFxWidget.h"
#include "AssetAssetRefWidget.h"
#include "AssetGroupWidget.h"
#include "AssetPathWidget.h"

#include "ActionContainer.h"
#include "ActionManager.h"
#include "Command.h"

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

void AssetProperty::appendGui(assetDb::AssetDB& db, IAssetEntry* pAssEntry, QWidget* pParent, 
	QGridLayout* pLayout, int32_t& row, int32_t depth)
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
	case PropertyType::VIDEO:
		pVideoWidget_ = new AssetVideoWidget(pParent, pAssEntry, val);
		break;
	case PropertyType::FONT:
		pFontWidget_ = new AssetFontWidget(pParent, pAssEntry, val);
		break;
	case PropertyType::IMAGE:
		pTextureWidget_ = new AssetTextureWidget(pParent, pAssEntry, val);
		break;
	case PropertyType::MODEL:
		pModelWidget_ = new AssetModelWidget(pParent, pAssEntry, val);
		break;
	case PropertyType::ANIM:
		pAnimWidget_ = new AssetAnimWidget(pParent, pAssEntry, val);
		break;
	case PropertyType::FX:
		pFxWidget_ = new AssetFxWidget(pParent, pAssEntry, val);
		break;
	case PropertyType::ASSET_REF:
		pAssetRefWidget_ = new AssetAssetRefWidget(pParent, db, pAssEntry, defaultValue_, val);
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
	case PropertyType::LABEL:
		pLabelWidget_ = new QLabel(pParent);
		pLabelWidget_->setText(QString::fromStdString(val));
		if (settings_.IsSet(Setting::BOLD_TEXT))
		{
			auto font = pLabelWidget_->font();
			font.setBold(true);
			pLabelWidget_->setFont(font);
		}

		if(fontCol_ != QColor())
		{
			pLabelWidget_->setStyleSheet(QString("QLabel { color : rgb(%1, %2, %3); }").arg(fontCol_.red()).arg(fontCol_.green()).arg(fontCol_.blue()));
		}

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
	case PropertyType::FONT:
	case PropertyType::IMAGE:
	case PropertyType::MODEL:
	case PropertyType::ANIM:
	case PropertyType::FX:
	case PropertyType::ASSET_REF:
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

		if (settings_.IsSet(Setting::BOLD_TEXT))
		{
			auto font = pCheckBoxWidget_->font();
			font.setBold(true);
			pCheckBoxWidget_->setFont(font);
		}

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


		pGroupWidget_->appendGui(db, pAssEntry, pParent, pLayout, row, depth);
	}
	else
	{	// add label.
		pLabel_ = new QLabel();
		pLabel_->setText(title);
		pLabel_->setToolTip(toolTip);
		
		if (settings_.IsSet(Setting::BOLD_TEXT))
		{
			auto font = pLabel_->font();
			font.setBold(true);
			pLabel_->setFont(font);
		}

		pLayout->addWidget(pLabel_, row, depth, 1, colSpanForCol(depth));
		pLayout->addWidget(pWidget_, row, MAX_COL_IDX);
	}

	if (!settings_.IsSet(Setting::ENABLED)) {
		enable(false);
	}
	if (!isVisible()) {
		show(false);
	}
	// re refreshing ui, re apply modified style if set.
	if (settings_.IsSet(Setting::MODIFIED)) {
		setModifiedStyle(true);
	}
}

void AssetProperty::valueChanged(const std::string& value)
{
	X_LOG0("AssetProperty", "newVal: %s", value.c_str());

	// so this prop was changed :D
	bool isModified = strSavedValue_ != value;
	SetValue(value);
	SetModified(isModified);

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
	if (type_ == PropertyType::GROUPBOX && pGroupWidget_) {
		pGroupWidget_->clear();
	}

	if (pWidget_) {
		delete pWidget_;
	}
	if (pLabel_) {
		delete pLabel_;
	}

	pWidget_ = nullptr;
	pLabel_ = nullptr;
}


void AssetProperty::clearUI(void)
{
	if (type_ == PropertyType::GROUPBOX) {
		pGroupWidget_->clearUI();
	//	return;
	}

	if (pWidget_) {
		delete pWidget_;
	}
	if (pLabel_) {
		delete pLabel_;
	}

	pWidget_ = nullptr;
	pLabel_ = nullptr;
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
	case PropertyType::INT:
	case PropertyType::FLOAT:
	case PropertyType::PATH:
	case PropertyType::TEXT:
	case PropertyType::VIDEO:
	case PropertyType::FONT:
	case PropertyType::IMAGE:
	case PropertyType::MODEL:
	case PropertyType::ANIM:
	case PropertyType::FX:
	case PropertyType::ASSET_REF:
	case PropertyType::STRING:
	case PropertyType::COLOR:
	case PropertyType::COMBOBOX:
	case PropertyType::LABEL:
		pWidget_->setVisible(vis);
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
	case PropertyType::INT:
	case PropertyType::FLOAT:
	case PropertyType::PATH:
	case PropertyType::TEXT:
	case PropertyType::VIDEO:
	case PropertyType::FONT:
	case PropertyType::IMAGE:
	case PropertyType::MODEL:
	case PropertyType::ANIM:
	case PropertyType::FX:
	case PropertyType::ASSET_REF:
	case PropertyType::STRING:
	case PropertyType::COLOR:
	case PropertyType::COMBOBOX:
	case PropertyType::GROUPBOX:
	case PropertyType::LABEL:
		pWidget_->setEnabled(val);
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

		// move the value to saved value.
		strSavedValue_ = strValue_;

		settings_.Remove(Setting::MODIFIED);
	}
	else {
		if (!modified) {
			return;
		}

		settings_.Set(Setting::MODIFIED);
	}

	setModifiedStyle(modified);
}

void AssetProperty::setModifiedStyle(bool modified)
{
	if (type_ == PropertyType::CHECKBOX && pCheckBoxWidget_)
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

void AssetProperty::collapseAll(void)
{
	if (type_ == PropertyType::GROUPBOX) {
		pGroupWidget_->collapseAll();
	}
}

void AssetProperty::expandAll(void)
{
	if (type_ == PropertyType::GROUPBOX) {
		pGroupWidget_->expandAll();
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

bool AssetProperty::HasChild(const AssetProperty& prop) const
{
	if (type_ == PropertyType::GROUPBOX) {
		return pGroupWidget_->HasChild(prop);
	}

	X_ASSERT_UNREACHABLE();
	return false;
}

bool AssetProperty::RemoveChild(const AssetProperty& prop)
{
	if (type_ == PropertyType::GROUPBOX) {
		return pGroupWidget_->RemoveChild(prop);
	}

	X_ASSERT_UNREACHABLE();
	return false;
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

void AssetProperty::SetSavedValue(const std::string& val)
{
	strValue_ = val;
	strSavedValue_ = val;
}

void AssetProperty::SetDefaultValue(const std::string& val)
{
	defaultValue_ = val;

	if (isNewProp()) {
		// set the saved value to the default?
		// this kinda depends how we want to handle new data
		// since args are complelty empty untill props are saved.
		// maybe mark everything that's not blank as modified intially?
		SetSavedValue(val);
		settings_.Remove(Setting::NEW_PROP);
	}
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

bool AssetProperty::isUpdateOnChange(void) const
{
	return settings_.IsSet(Setting::UPDATE_ON_CHANGE);
}

bool AssetProperty::isVisible(void) const
{
	if (!parentKey_.empty())
	{
		// we should check if parent is visible :|
		// but how do we find him!

	}

	return settings_.IsSet(Setting::VISIBLE);
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
		
	pLayout_ = new QGridLayout();
	pCon_ = new QWidget(widget);
	pCon_->setObjectName("AssetProperyEditor");

	pCon_->setLayout(pLayout_);
	pWidget_->setWidget(pCon_);
	pWidget_->setWidgetResizable(true);
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
	if (AssetPropertyEditorWidget* wid = qobject_cast<AssetPropertyEditorWidget*>(widget)) {
		pWidget_ = wid;
	}
}

bool AssetProperties::save(QString& errorString)
{
	const core::string& assetName = nameNarrow();
	const auto& type = type_;

	if (assetName.isEmpty()) {
		errorString = "Error saving asset props, name is invalid";
		return false;
	}

	if (!isModified()) {
		X_WARNING("AssetProp", "Saving props when they are not modified.");
	}

	core::string args;
	if (!extractArgs(args)) {
		errorString = "Error extracting asset '" + assetName + "' props";
		return false;
	}

	auto res = db_.UpdateAssetArgs(type, assetName, args);
	if (res != assetDb::AssetDB::Result::OK) {
		errorString = "Failed to save asset '" + assetName + "' props. Error: " + assetDb::AssetDB::Result::ToString(res);
		return false;
	}
	else {
		X_LOG2("AssetProp", "Saved \"%s\" props", assetName.c_str());
	}

	// meow meow meow.
	setNotModifiedModified();
	return true;
}


bool AssetProperties::updateRawFile(const ByteArr& compressedData)
{
	const core::string& assetName = nameNarrow();

	auto res = db_.UpdateAssetRawFile(type(), assetName, compressedData);
	if (res != assetDb::AssetDB::Result::OK && res != assetDb::AssetDB::Result::UNCHANGED) {
		X_ERROR("AssetProp", "Failed to update raw file for asset. Err: %s", assetDb::AssetDB::Result::ToString(res));
		return false;
	}
	else {
		X_LOG2("AssetProp", "Updated raw file for \"%s\" props", assetName.c_str());
	}

	return true;
}


bool AssetProperties::getRawFile(ByteArr& rawData)
{
	const core::string& assetName = nameNarrow();

	int32_t assetId;
	if (!db_.AssetExsists(type(), assetName, &assetId)) {
		X_ERROR("AssetProp", "Asset `%s` does not exsist", assetName.c_str());
		return false;
	}

	bool res = db_.GetRawFileDataForAsset(assetId, rawData);
	if (!res) {
		X_ERROR("AssetProp", "Failed to get raw file for asset.");
		return false;
	}

	return true;
}


bool AssetProperties::updateThumb(const ByteArr& data, Vec2i thumbDim, Vec2i srcDim,
	core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl)
{
	const core::string& assetName = nameNarrow();

	auto res = db_.UpdateAssetThumb(type(), assetName, thumbDim, srcDim, data, algo, lvl);
	if (res != assetDb::AssetDB::Result::OK && res != assetDb::AssetDB::Result::UNCHANGED) {
		X_ERROR("AssetProp", "Failed to update thumb for asset. Err: %s", assetDb::AssetDB::Result::ToString(res));
		return false;
	}
	else 
	{
		X_LOG2("AssetProp", "Updated thumb for \"%s\" props", assetName.c_str());
	}
	
	return true;
}

bool AssetProperties::getThumb(ByteArr& data, Vec2i& dim)
{
	const core::string& assetName = nameNarrow();

	int32_t assetId;
	if (!db_.AssetExsists(type(), assetName, &assetId)) {
		X_ERROR("AssetProp", "Asset `%s` does not exsist", assetName.c_str());
		return false;
	}

	assetDb::AssetDB::ThumbInfo info;
	bool res = db_.GetThumbForAsset(assetId, info, data);
	if (!res) {
		X_WARNING("AssetProp", "Failed to get thumb for asset.");
		return false;
	}
	else {
		X_LOG2("AssetProp", "Found thumb for \"%s\" props", assetName.c_str());
	}

	dim = info.srcDim;
	return true;
}


bool AssetProperties::reloadUi(void)
{
	const core::string& assetName = nameNarrow();
	X_LOG0("AssetProperties", "UI script refresh request for: \"%s\" type: %s", assetName.c_str(), assetDb::AssetType::ToString(type()));

	pCon_->setUpdatesEnabled(false);

	pCur_ = nullptr;
	root_.clearUI();
	root_.SetType(AssetProperty::PropertyType::GROUPBOX);

	for (auto it = keys_.begin(); it != keys_.end(); ++it) {
		(*it)->clearUI();
	}

	// run again
	if (!pPropScriptMan_->runScriptForProps(*this, type())) {
		return false;
	}

	appendGui(pCon_, pLayout_);
	pCon_->setUpdatesEnabled(true);

	return true;
}

bool AssetProperties::isModified(void) const
{
	// humm use the cached value or really check.
	for (auto it = keys_.begin(); it != keys_.end(); ++it) {
		if ((*it)->isModified()) {
			return true;
		}
	}

	return false;
}

bool AssetProperties::isSaveAsAllowed(void) const
{
	return false;
}

void AssetProperties::collapseAll(void)
{
	for (const auto& pChild : root_) {
		pChild->collapseAll();
	}
}

void AssetProperties::expandAll(void)
{
	for (const auto& pChild : root_) {
		pChild->expandAll();
	}
}


bool AssetProperties::loadProps(QString& errorString, const QString& assetName, assetDb::AssetType::Enum type)
{
	// we want to load the args from the db.
	const auto narrowName = assetName.toLocal8Bit();
	const core::string name(narrowName.data());

	if (name.isEmpty()) {
		errorString = "Can't load props for asset with blank name";
		return false;
	}

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

	if (!pPropScriptMan_->runScriptForProps(*this, type)) {
		errorString = "Error running property script for asset '" + assetName + "'";
		return false;
	}

	// QWidget* pCon = new QWidget();
	// QGridLayout* pLayout = new QGridLayout();
	pCon_->setUpdatesEnabled(false);
	appendGui(pCon_, pLayout_);
	pCon_->setUpdatesEnabled(true);


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

		std::string strVal;

		switch (val.GetType())
		{
		case core::json::Type::kFalseType:
			strVal = "0";
			break;
		case core::json::Type::kTrueType:
			strVal = "1";
			break;
		case core::json::Type::kStringType:
			strVal = std::string(val.GetString(), val.GetStringLength());
			break;
		case core::json::Type::kNumberType:
			if (val.IsBool()) {
				strVal = val.GetBool() ? "1" : "0";
			}
			if (val.IsInt()) {
				strVal = std::to_string(val.GetInt());
			}
			else if (val.IsFloat()) {
				strVal = std::to_string(val.GetFloat());
			}
			else {
				// default to double
				strVal = std::to_string(val.GetDouble());
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

		item.SetSavedValue(strVal);
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

		// we don't save label's in props data.
		if (item.GetType() == AssetProperty::PropertyType::LABEL) {
			continue;
		}

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
		case AssetProperty::PropertyType::FONT:
		case AssetProperty::PropertyType::IMAGE:
		case AssetProperty::PropertyType::MODEL:
		case AssetProperty::PropertyType::ANIM:
		case AssetProperty::PropertyType::FX:
		case AssetProperty::PropertyType::ASSET_REF:
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



bool AssetProperties::appendGui(QWidget* pParent, QGridLayout* pLayout)
{
	for (int32_t i = 0; i < 12; i++) {
		pLayout->setColumnMinimumWidth(i, 16);
	}


	int32_t row = 0;
	int32_t depth = 0;

	for (const auto& pChild : root_)
	{
		pChild->appendGui(db_, this, pParent, pLayout, row, depth);
		row += 1;
	}

	pLayout->setRowStretch(row, 1);
	pLayout->setColumnStretch(AssetProperty::MAX_COL_IDX, 1);
//	pLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), row + 1, 0, 1, -1);
	return true;
}


void AssetProperties::setNotModifiedModified(void)
{
	// ya fucking wut.
	bool wasModified = false;

	for (auto it = keys_.begin(); it != keys_.end(); ++it) 
	{
		wasModified |= (*it)->isModified();
		(*it)->SetModified(false);
	}

	if (wasModified) {
		emit modificationChanged(false);
	}
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

			connect(pItem, SIGNAL(modified(void)), this, SLOT(propModified(void)));

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

		if (type != AssetProperty::PropertyType::LABEL) {
			pItem->SetModified(true); // if it's a new prop show that the props need saving for this new props to get added to db.
	
			 // CONNECT :D !
			connect(pItem, SIGNAL(modified(void)), this, SLOT(propModified(void)));
		}

		keys_[key] = pItem;

	}

	pCur_->AddChild(pItem);

	return *pItem;
}

void AssetProperties::showInCurrentCat(const AssetProperty& prop)
{
	// would this would work better if props where not in a tree.
	// but the gui was still created in a tree.
	// then we could just change hte props cat name
	// dunno if that would making showing them more of a pain tho..
	// or props could keep a pointer to the cat they are in.
	// that way when we iterate props for creating the ui
	// we just add the ui to the group it's pointing at.
	// would mean a refactor :) !


	X_ASSERT_NOT_NULL(pCur_);

	// so first we need to know if item is not in this cat.
	if (!pCur_->HasChild(prop))
	{
		// we need to move it to this cat :(
		// how do we find it ?
		// maybe we should only just support moving for items in the root cat.
		// it's the use case we are looking for so sounds good to me.

		if (root_.begin() == root_.end())
		{
			return;
		}

		AssetProperty* pDefaultGRoup = *root_.begin();

		if (!pDefaultGRoup->RemoveChild(prop))
		{

			return;
		}

		// add it. const_cast kinda feels right since this function won't edit it.
		// but later the group might edit it hence why it's none const.
		pCur_->AddChild(const_cast<AssetProperty*>(&prop));
	}
}

void AssetProperties::propModified(void)
{
	// a prop was modified.
	AssetProperty* pProp = qobject_cast<AssetProperty*>(sender());
	if (pProp)
	{
		// now we don't want to count a props that's modified twice
		// so it needs to be per prop.
		bool modified = isModified();

		emit modificationChanged(modified);
		emit changed();

		if (pProp->isUpdateOnChange())
		{
			X_LOG0("Prop", "Update on change requted by: %s", pProp->GetKey().c_str());

			// we can't reload the ui here as we are in a slot for a widget we may delete so it must be delayed.
			QMetaObject::invokeMethod(this, "reloadUiSlot", Qt::QueuedConnection);
		}
	}
}

void AssetProperties::reloadUiSlot(void)
{
	const bool res = reloadUi();
	X_UNUSED(res);
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
	ctor(QSharedPointer<AssetProperties>(new AssetProperties(db, pPropScriptMan, this)));
}

AssetPropertyEditorWidget::AssetPropertyEditorWidget(assetDb::AssetDB& db, AssetPropsScriptManager* pPropScriptMan, AssetProperties* pAssetEntry, QWidget *parent) :
	QScrollArea(parent),
	db_(db),
	pPropScriptMan_(pPropScriptMan),
	pEditor_(nullptr)
{
	ctor(QSharedPointer<AssetProperties>(pAssetEntry));
}

AssetPropertyEditorWidget::AssetPropertyEditorWidget(AssetPropertyEditorWidget* pOther) :
	db_(pOther->db_),
	pPropScriptMan_(pOther->pPropScriptMan_),
	pEditor_(nullptr)
{
	ctor(pOther->assetProps_);
}

AssetPropertyEditorWidget::~AssetPropertyEditorWidget()
{

}

void AssetPropertyEditorWidget::ctor(const QSharedPointer<AssetProperties>& props)
{
	setObjectName("AssetProperyEditor");

	assetProps_ = props;

	connect(assetProps_.data(), SIGNAL(modificationChanged(bool)),
		this, SIGNAL(modificationChanged(bool)));
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


bool AssetPropertyEditorWidget::isUndoAvailable(void) const
{
	return false;
}

bool AssetPropertyEditorWidget::isRedoAvailable(void) const
{
	return false;
}

bool AssetPropertyEditorWidget::isReadOnly(void) const
{
	return false;
}

void AssetPropertyEditorWidget::undo(void)
{

}

void AssetPropertyEditorWidget::redo(void)
{

}

void AssetPropertyEditorWidget::copy(void)
{

}

void AssetPropertyEditorWidget::cut(void)
{

}

void AssetPropertyEditorWidget::paste(void)
{

}

void AssetPropertyEditorWidget::collapseAll(void)
{
	assetProps_->collapseAll();
}

void AssetPropertyEditorWidget::expandAll(void)
{
	assetProps_->expandAll();
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

void AssetPropertyEditorWidget::contextMenuEvent(QContextMenuEvent* e)
{
	showDefaultContextMenu(e, Constants::ASSETPROP_EDITOR_CONTEXT);
}

void AssetPropertyEditorWidget::showDefaultContextMenu(QContextMenuEvent* e, const Id menuContextId)
{
	QMenu menu;
	appendMenuActionsFromContext(&menu, menuContextId);
	appendStandardContextMenuActions(&menu);
	menu.exec(e->globalPos());
}



void AssetPropertyEditorWidget::appendMenuActionsFromContext(QMenu *menu, const Id menuContextId)
{
	ActionContainer *mcontext = ActionManager::actionContainer(menuContextId);
	if (mcontext) {
		QMenu* contextMenu = mcontext->menu();

		foreach(QAction *action, contextMenu->actions()) {
			menu->addAction(action);
		}
	}
}

void AssetPropertyEditorWidget::appendStandardContextMenuActions(QMenu *menu)
{
	menu->addSeparator();

	QAction* a = ActionManager::command(Constants::EDIT_UNDO)->action();
	menu->addAction(a);
	a = ActionManager::command(Constants::EDIT_REDO)->action();
	menu->addAction(a);

	menu->addSeparator();

	a = ActionManager::command(Constants::EDIT_CUT)->action();
	menu->addAction(a);
	a = ActionManager::command(Constants::EDIT_COPY)->action();
	menu->addAction(a);
	a = ActionManager::command(Constants::EDIT_PASTE)->action();
	menu->addAction(a);

	menu->addSeparator();

	a = ActionManager::command(Constants::ASSETPROP_COLLAPSE_ALL)->action();
	menu->addAction(a);
	a = ActionManager::command(Constants::ASSETPROP_UNCOLLAPSE_ALL)->action();
	menu->addAction(a);
}



// ----------------------------------------------------------------


AssetPropertyEditor::AssetPropertyEditor(AssetPropertyEditorWidget* editor) :
	pEditorWidget_(editor)
{
	setWidget(pEditorWidget_);

	setContext(Context(Constants::C_ASSETPROP_EDITOR));

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

bool AssetPropertyEditor::duplicateSupported(void) const
{
	// could be supported, just need to setup widgets and shit for duplicated
	// prop view.
	return false;
}

IEditor* AssetPropertyEditor::duplicate(void)
{
	AssetPropertyEditorWidget* ret = new AssetPropertyEditorWidget(pEditorWidget_);

	return ret->editor();
}

void AssetPropertyEditor::modificationChanged(bool modified)
{
	QString title = assetEntry()->displayName();
	X_UNUSED(modified);

	emit titleChanged(title);
}

X_NAMESPACE_END