#include "AssetModelWidget.h"

#include <IModel.h>
#include "IAssetEntry.h"


X_NAMESPACE_BEGIN(editor)

AssetModelWidget::AssetModelWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value) :
	QWidget(parent),
	pAssEntry_(pAssEntry),
	pProgress_(nullptr),
	pWarningAction_(nullptr),
	loader_(g_arena, assetDb::AssetType::MODEL)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);

	pEditPath_ = new QLineEdit();
	pEditPath_->installEventFilter(this);

	connect(pEditPath_, SIGNAL(editingFinished()), this, SLOT(editingFinished()));

	// browse button
	QToolButton* pBrowse = new QToolButton();
	pBrowse->setText("...");
	setPromptDialogFilter("Raw Model(*.model_raw)");

	connect(pBrowse, SIGNAL(clicked()), this, SLOT(browseClicked()));

	pLayout->addWidget(pEditPath_);
	pLayout->addWidget(pBrowse);

	setLayout(pLayout);


	connect(&loader_, &RawFileLoader::setProgress, this, &AssetModelWidget::setProgress, Qt::QueuedConnection);
	connect(&loader_, &RawFileLoader::setProgressLabel, this, &AssetModelWidget::setProgressLabel, Qt::QueuedConnection);
	connect(&loader_, SIGNAL(finished()), this, SLOT(rawFileLoaded()));


	setValue(value);
}


AssetModelWidget::~AssetModelWidget()
{

}


void AssetModelWidget::setPromptDialogTitle(const QString& title)
{
	dialogTitleOverride_ = title;
}

QString AssetModelWidget::promptDialogTitle(void) const
{
	return dialogTitleOverride_;
}

void AssetModelWidget::setPromptDialogFilter(const QString& filter)
{
	dialogFilter_ = filter;
}

QString AssetModelWidget::promptDialogFilter(void) const
{
	return dialogFilter_;
}

QString AssetModelWidget::path(void) const
{
	return curPath_;
}

void AssetModelWidget::setPath(const QString& filePath)
{
	if (filePath == curPath_) {
		return;
	}

	curPath_ = QDir::toNativeSeparators(filePath);
	pEditPath_->setText(curPath_);

	validatePath();

	emit valueChanged(curPath_.toStdString());
}

void AssetModelWidget::setErrorTip(const QString& tip)
{
	if (!pWarningAction_) {
		QIcon icon(":/misc/img/warning_32.png");
		pWarningAction_ = pEditPath_->addAction(icon, QLineEdit::ActionPosition::LeadingPosition);
		pWarningAction_->setToolTip(tip);
	}
}

void AssetModelWidget::removeErrorTip(void)
{
	if (pWarningAction_) {
		delete pWarningAction_;
		pWarningAction_ = nullptr;
	}
}

bool AssetModelWidget::loadRawModel(void)
{
	const QString& curPath = curPath_;
	QFileInfo fi(curPath);

	bool showProgress = fi.size() > 1024 * 512;

	if (!pProgress_ && showProgress) {
		pProgress_ = new QProgressDialog("Importing..", "Cancel", 0, 30, ICore::mainWindow());
		pProgress_->setWindowModality(Qt::WindowModal);
		pProgress_->setCancelButton(nullptr);
		pProgress_->show();
	}

	loader_.loadFile(curPath);
	return true;
}

void AssetModelWidget::setValue(const std::string& value)
{
	blockSignals(true);
	pEditPath_->blockSignals(true);

	{
		curPath_ = QDir::toNativeSeparators(QString::fromStdString(value));
		pEditPath_->setText(curPath_);
	}

	pEditPath_->blockSignals(false);
	blockSignals(false);
}

void AssetModelWidget::editingFinished(void)
{
	setPath(pEditPath_->text());
}


void AssetModelWidget::validatePath(void)
{
	const QString& curPath = curPath_;

	// if path not empty try load it.
	if (curPath.isEmpty()) {
		return;
	}

	// do some stuff like validate extensions.
	// check it's valid etc..
	QFileInfo fi(curPath);
	if (!fi.exists())
	{
		setErrorTip("File does not exist");
		return;
	}
	else if (!fi.isFile())
	{
		setErrorTip("Path is not a file");
		return;
	}
	else if (!fileExtensionValid(curPath_))
	{
		setErrorTip("Invalid file extension");
		return;
	}

	removeErrorTip();

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	if (!loadRawModel()) {


	}

	QApplication::restoreOverrideCursor();
}

void AssetModelWidget::browseClicked(void)
{
	QString predefined = path();

	QFileInfo fi(predefined);

	if (!predefined.isEmpty() && !fi.isDir()) {
		predefined = fi.path();
		fi.setFile(predefined);
	}

	if ((predefined.isEmpty() || !fi.isDir()) && !initialBrowsePathOverride_.isNull()) {
		predefined = initialBrowsePathOverride_;
		fi.setFile(predefined);
		if (!fi.isDir()) {
			predefined.clear();
			fi.setFile(QString());
		}
	}

	QString newPath = QFileDialog::getOpenFileName(this, makeDialogTitle(tr("Choose RawModel File")),
		predefined, dialogFilter_);

	if (!newPath.isEmpty())
	{
		setPath(newPath);
	}
}


void AssetModelWidget::setProgress(int32_t pro)
{
	if (pProgress_) {
		pProgress_->setValue(pro);
	}
}

void AssetModelWidget::setProgressLabel(const QString& label, int32_t pro)
{
	if (pProgress_) {
		pProgress_->setLabelText(label);
		pProgress_->setValue(pro);
	}
}

void AssetModelWidget::rawFileLoaded(void)
{
	if (pProgress_) {

		const bool canceled = pProgress_->wasCanceled();

		pProgress_->close();
		delete pProgress_;
		pProgress_ = nullptr;

		if (canceled) {
			return;
		}
	}


	const auto srcData = loader_.getCompressedSrc();

	// src data
	pAssEntry_->updateRawFile(srcData);
}

bool AssetModelWidget::eventFilter(QObject* pObject, QEvent* pEvent)
{
	const auto type = pEvent->type();

	if (pObject == pEditPath_)
	{
		if (type == QEvent::DragEnter)
		{
			dragEnterEvent(static_cast<QDragEnterEvent*>(pEvent));
		}
		else if (type == QEvent::Drop)
		{
			dropEvent(static_cast<QDropEvent*>(pEvent));
			return true;
		}
	}

	return false;
}

void AssetModelWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls()) {
		auto urls = event->mimeData()->urls();
		if (urls.size() == 1)
		{
			const auto& url = urls[0];
			if (url.isLocalFile())
			{
				QString filePath = url.toLocalFile();

				if (fileExtensionValid(filePath))
				{
					event->setDropAction(Qt::DropAction::LinkAction);
					event->accept();
				}
			}
		}
	}
}

void AssetModelWidget::dropEvent(QDropEvent* event)
{
	if (event->mimeData()->hasUrls())
	{
		auto urls = event->mimeData()->urls();
		if (urls.size() == 1)
		{
			const auto& url = urls[0];
			if (url.isLocalFile())
			{
				// ye foooking wut.
				QString filePath = url.toLocalFile();

				// shieeeet.
				// we might want to add support for making this relative to base dir... humm
				// for now we just slap a cow.
				filePath = QDir::toNativeSeparators(filePath);

				setPath(filePath);

				event->accept();
			}
		}
	}
}

QString AssetModelWidget::makeDialogTitle(const QString& title)
{
	if (dialogTitleOverride_.isNull()) {
		return title;
	}

	return dialogTitleOverride_;
}


bool AssetModelWidget::fileExtensionValid(const QString& path)
{
	if (path.endsWith(model::MODEL_RAW_FILE_EXTENSION, Qt::CaseInsensitive))
	{
		return true;
	}

	return false;
}


X_NAMESPACE_END