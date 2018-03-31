#include "stdafx.h"
#include "FxEditor.h"
#include "AssetFxWidget.h"

#include "ActionContainer.h"
#include "ActionManager.h"
#include "Command.h"

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(assman)



// -------------------------------------------------------------------------

FxProperties::FxProperties(assetDb::AssetDB& db, FxEditorWidget* widget) :
	IAssetEntry(widget),
	db_(db),
	pWidget_(widget)
{
	pEditorWidget_ = new AssetFxWidget(this);

	pLayout_ = new QVBoxLayout();
	pLayout_->addWidget(pEditorWidget_);

	pCon_ = new QWidget(widget);
	pCon_->setObjectName("FxEditor");
	pCon_->setLayout(pLayout_);

	connect(pEditorWidget_, &AssetFxWidget::valueChanged, this, &FxProperties::valueChanged);

	pWidget_->setWidget(pCon_);
	pWidget_->setWidgetResizable(true);
}

FxProperties::~FxProperties()
{

}


void FxProperties::setWidget(QWidget* widget)
{
	if (FxEditorWidget* wid = qobject_cast<FxEditorWidget*>(widget)) {
		pWidget_ = wid;
	}
}

bool FxProperties::save(QString& errorString)
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

	auto res = db_.UpdateAssetArgs(type, assetName, current_);
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


bool FxProperties::updateRawFile(const ByteArr& compressedData)
{
	X_UNUSED(compressedData);

	return false;
}


bool FxProperties::getRawFile(ByteArr& rawData)
{
	X_UNUSED(rawData);

	return false;
}

bool FxProperties::updateThumb(const ByteArr& data, Vec2i thumbDim, Vec2i srcDim,
	core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl)
{
	X_UNUSED(data, thumbDim, srcDim, algo, lvl);

	return false;
}

bool FxProperties::getThumb(ByteArr& data, Vec2i& dim)
{
	X_UNUSED(data, dim);

	return false;
}


bool FxProperties::reloadUi(void)
{
	return false;
}

bool FxProperties::isModified(void) const
{
	return saved_ != current_;
}

bool FxProperties::isSaveAsAllowed(void) const
{
	return false;
}


bool FxProperties::loadProps(QString& errorString, const QString& assetName, assetDb::AssetType::Enum type)
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

	if (!db_.GetArgsForAsset(assetId, saved_)) {
		errorString = "Failed to get asset '" + assetName + "' props";
		return false;
	}

	current_ = saved_;

	pEditorWidget_->setUpdatesEnabled(false);
	bool res = pEditorWidget_->setValue(saved_);
	pEditorWidget_->setUpdatesEnabled(true);

	return res;
}


void FxProperties::setNotModifiedModified(void)
{
	// if we where modified, emit a change.
	const bool wasModified = isModified();

	if (wasModified) {
		current_ = saved_;
		emit modificationChanged(false);

		X_ASSERT(!isModified(), "Should not be modified")(isModified());
	}
}

FxEditorWidget* FxProperties::getEditor(void)
{
	return pWidget_;
}

void FxProperties::valueChanged(void)
{
	pEditorWidget_->getValue(current_);

	bool modified = isModified();

	emit modificationChanged(modified);
	emit changed();

}
// ----------------------------------------------------------------




FxEditorWidget::FxEditorWidget(assetDb::AssetDB& db, QWidget *parent) :
	QScrollArea(parent),
	db_(db),
	pEditor_(nullptr)
{
	ctor(QSharedPointer<FxProperties>(new FxProperties(db, this)));
}

FxEditorWidget::FxEditorWidget(assetDb::AssetDB& db, FxProperties* pAssetEntry, QWidget *parent) :
	QScrollArea(parent),
	db_(db),
	pEditor_(nullptr)
{
	ctor(QSharedPointer<FxProperties>(pAssetEntry));
}


FxEditorWidget::FxEditorWidget(FxEditorWidget* pOther) :
	db_(pOther->db_),
	pEditor_(nullptr)
{
	ctor(pOther->fxProps_);
}

FxEditorWidget::~FxEditorWidget()
{

}

void FxEditorWidget::ctor(const QSharedPointer<FxProperties>& props)
{
	setObjectName("FxEditor");

	fxProps_ = props;

	connect(fxProps_.data(), SIGNAL(modificationChanged(bool)), this, SIGNAL(modificationChanged(bool)));
}


bool FxEditorWidget::open(QString* pErrorString, const QString& fileName, assetDb::AssetType::Enum type)
{
	X_UNUSED(pErrorString);
	X_UNUSED(fileName);
	X_UNUSED(type);

	FxProperties* pProbs = fxProps_.data();
	pProbs->setType(type);
	pProbs->setAssetName(fileName);
	if (!pProbs->loadProps(*pErrorString, fileName, type)) {
		return false;
	}

	return true;
}


bool FxEditorWidget::isUndoAvailable(void) const
{
	return false;
}

bool FxEditorWidget::isRedoAvailable(void) const
{
	return false;
}

bool FxEditorWidget::isReadOnly(void) const
{
	return false;
}

void FxEditorWidget::undo(void)
{

}

void FxEditorWidget::redo(void)
{

}

void FxEditorWidget::copy(void)
{

}

void FxEditorWidget::cut(void)
{

}

void FxEditorWidget::paste(void)
{

}


FxEditor* FxEditorWidget::editor(void)
{
	if (!pEditor_) {
		pEditor_ = createEditor();
	}

	return pEditor_;
}

FxProperties* FxEditorWidget::fxProperties(void) const
{
	return fxProps_.data();
}

FxEditor* FxEditorWidget::createEditor(void)
{
	return new FxEditor(this);
}

void FxEditorWidget::contextMenuEvent(QContextMenuEvent* e)
{
	showDefaultContextMenu(e, Constants::ASSETPROP_EDITOR_CONTEXT);
}

void FxEditorWidget::showDefaultContextMenu(QContextMenuEvent* e, const Id menuContextId)
{
	QMenu menu;
	appendMenuActionsFromContext(&menu, menuContextId);
	appendStandardContextMenuActions(&menu);
	menu.exec(e->globalPos());
}



void FxEditorWidget::appendMenuActionsFromContext(QMenu *menu, const Id menuContextId)
{
	ActionContainer *mcontext = ActionManager::actionContainer(menuContextId);
	if (mcontext) {
		QMenu* contextMenu = mcontext->menu();

		foreach(QAction *action, contextMenu->actions()) {
			menu->addAction(action);
		}
	}
}

void FxEditorWidget::appendStandardContextMenuActions(QMenu *menu)
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


FxEditor::FxEditor(FxEditorWidget* editor) :
	pEditorWidget_(editor)
{
	setWidget(pEditorWidget_);

	setContext(Context(Constants::C_FX_EDITOR));

	connect(editor, SIGNAL(modificationChanged(bool)), this, SLOT(modificationChanged(bool)));

}

FxEditor::~FxEditor()
{
	delete pEditorWidget_;
}


bool FxEditor::open(QString* pErrorString, const QString& fileName, assetDb::AssetType::Enum type)
{
	return pEditorWidget_->open(pErrorString, fileName, type);
}

IAssetEntry* FxEditor::assetEntry(void)
{
	return pEditorWidget_->fxProperties();
}

Id FxEditor::id(void) const
{
	return Id(Constants::FX_EDITOR_ID);
}

bool FxEditor::duplicateSupported(void) const
{
	// could be supported, just need to setup widgets and shit for duplicated
	// prop view.
	return false;
}

IEditor* FxEditor::duplicate(void)
{
	FxEditorWidget* ret = new FxEditorWidget(pEditorWidget_);

	return ret->editor();
}

void FxEditor::modificationChanged(bool modified)
{
	QString title = assetEntry()->displayName();
	X_UNUSED(modified);

	emit titleChanged(title);
}

X_NAMESPACE_END