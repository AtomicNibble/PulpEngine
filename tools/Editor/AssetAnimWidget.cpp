#include "AssetAnimWidget.h"

#include <IAnimation.h>
#include "IAssetEntry.h"


X_NAMESPACE_BEGIN(editor)

AssetAnimWidget::AssetAnimWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value) :
	QWidget(parent),
	pAssEntry_(pAssEntry),
	pProgress_(nullptr),
	pWarningAction_(nullptr),
	loader_(g_arena, assetDb::AssetType::ANIM)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);

	pEditPath_ = new QLineEdit();
	pEditPath_->installEventFilter(this);

	connect(pEditPath_, SIGNAL(editingFinished()), this, SLOT(editingFinished()));

	// browse button
	QToolButton* pBrowse = new QToolButton();
	pBrowse->setText("...");
	setPromptDialogFilter("Raw Anim(*.anim_inter)");

	connect(pBrowse, SIGNAL(clicked()), this, SLOT(browseClicked()));

	pLayout->addWidget(pEditPath_);
	pLayout->addWidget(pBrowse);

	setLayout(pLayout);


	connect(&loader_, &RawFileLoader::setProgress, this, &AssetAnimWidget::setProgress, Qt::QueuedConnection);
	connect(&loader_, &RawFileLoader::setProgressLabel, this, &AssetAnimWidget::setProgressLabel, Qt::QueuedConnection);
	connect(&loader_, SIGNAL(finished()), this, SLOT(rawFileLoaded()));


	setValue(value);
}


AssetAnimWidget::~AssetAnimWidget()
{

}


void AssetAnimWidget::setPromptDialogTitle(const QString& title)
{
	dialogTitleOverride_ = title;
}

QString AssetAnimWidget::promptDialogTitle(void) const
{
	return dialogTitleOverride_;
}

void AssetAnimWidget::setPromptDialogFilter(const QString& filter)
{
	dialogFilter_ = filter;
}

QString AssetAnimWidget::promptDialogFilter(void) const
{
	return dialogFilter_;
}

QString AssetAnimWidget::path(void) const
{
	return curPath_;
}

void AssetAnimWidget::setPath(const QString& filePath)
{
	if (filePath == curPath_) {
		return;
	}

	curPath_ = QDir::toNativeSeparators(filePath);
	pEditPath_->setText(curPath_);

	validatePath();

	emit valueChanged(curPath_.toStdString());
}

void AssetAnimWidget::setErrorTip(const QString& tip)
{
	if (!pWarningAction_) {
		QIcon icon(":/misc/img/warning_32.png");
		pWarningAction_ = pEditPath_->addAction(icon, QLineEdit::ActionPosition::LeadingPosition);
		pWarningAction_->setToolTip(tip);
	}
}

void AssetAnimWidget::removeErrorTip(void)
{
	if (pWarningAction_) {
		delete pWarningAction_;
		pWarningAction_ = nullptr;
	}
}

bool AssetAnimWidget::loadRawModel(void)
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

void AssetAnimWidget::setValue(const std::string& value)
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

void AssetAnimWidget::editingFinished(void)
{
	setPath(pEditPath_->text());
}


void AssetAnimWidget::validatePath(void)
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

void AssetAnimWidget::browseClicked(void)
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

	QString newPath = QFileDialog::getOpenFileName(this, makeDialogTitle(tr("Choose Inter Anim File")),
		predefined, dialogFilter_);

	if (!newPath.isEmpty())
	{
		setPath(newPath);
	}
}


void AssetAnimWidget::setProgress(int32_t pro)
{
	if (pProgress_) {
		pProgress_->setValue(pro);
	}
}

void AssetAnimWidget::setProgressLabel(const QString& label, int32_t pro)
{
	if (pProgress_) {
		pProgress_->setLabelText(label);
		pProgress_->setValue(pro);
	}
}

void AssetAnimWidget::rawFileLoaded(void)
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

bool AssetAnimWidget::eventFilter(QObject* pObject, QEvent* pEvent)
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

void AssetAnimWidget::dragEnterEvent(QDragEnterEvent *event)
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

void AssetAnimWidget::dropEvent(QDropEvent* event)
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

QString AssetAnimWidget::makeDialogTitle(const QString& title)
{
	if (dialogTitleOverride_.isNull()) {
		return title;
	}

	return dialogTitleOverride_;
}


bool AssetAnimWidget::fileExtensionValid(const QString& path)
{
	if (path.endsWith(anim::ANIM_INTER_FILE_EXTENSION, Qt::CaseInsensitive))
	{
		return true;
	}

	return false;
}


X_NAMESPACE_END