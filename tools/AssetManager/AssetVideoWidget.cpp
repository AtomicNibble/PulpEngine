#include "stdafx.h"
#include "AssetVideoWidget.h"


#include "DisplayPixDialog.h"

#include "IAssetEntry.h"

X_NAMESPACE_BEGIN(assman)


// ----------------------------------


AssetVideoWidget::AssetVideoWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value) :
	QWidget(parent),
	pAssEntry_(pAssEntry),
	pProgress_(nullptr),
	loader_(g_arena, assetDb::AssetType::VIDEO)
{
	X_UNUSED(value);

	QPixmap imgPix(":/assetDb/img/Drop_video.png");
	pDropZone_ = new QLabel();
	pDropZone_->setPixmap(imgPix);
	pDropZone_->installEventFilter(this);
	pDropZone_->setAcceptDrops(true);

	// browse button
	QToolButton* pBrowse = new QToolButton();
	pBrowse->setText("...");
	setPromptDialogFilter("Video (*.ivf)");

	connect(pBrowse, SIGNAL(clicked()), this, SLOT(browseClicked()));

	//  P | hoz
	//  P
	QGridLayout* pLayout = new QGridLayout();
	{
		pLabel_ = new QLabel();
		pLabel_->installEventFilter(this);
		pLabel_->setAcceptDrops(true);

		QVBoxLayout* pVertLayout = new QVBoxLayout();
		pVertLayout->setContentsMargins(0, 0, 0, 0);

		QHBoxLayout* pHozLayout = new QHBoxLayout();
		pHozLayout->setContentsMargins(0, 0, 0, 0);
		pHozLayout->addWidget(pDropZone_);
		pHozLayout->addWidget(pLabel_, 2);
		pHozLayout->addWidget(pBrowse);

		pLayout->addLayout(pVertLayout, 0, 0, 2, 1);
		pLayout->addLayout(pHozLayout, 0, 1, 1, 1);
	}

	setLayout(pLayout);

	connect(&loader_, &RawFileLoader::setProgress, this, &AssetVideoWidget::setProgress, Qt::QueuedConnection);
	connect(&loader_, &RawFileLoader::setProgressLabel, this, &AssetVideoWidget::setProgressLabel, Qt::QueuedConnection);
	connect(&loader_, SIGNAL(finished()), this, SLOT(rawFileLoaded()));

	setValue(value);
}

AssetVideoWidget::~AssetVideoWidget()
{
}


void AssetVideoWidget::setPromptDialogTitle(const QString& title)
{
	dialogTitleOverride_ = title;
}

QString AssetVideoWidget::promptDialogTitle(void) const
{
	return dialogTitleOverride_;
}

void AssetVideoWidget::setPromptDialogFilter(const QString& filter)
{
	dialogFilter_ = filter;
}

QString AssetVideoWidget::promptDialogFilter(void) const
{
	return dialogFilter_;
}

void AssetVideoWidget::setValue(const std::string& value)
{
	const char* pFileName = core::strUtil::FileName(value.c_str());

	pLabel_->setText(pFileName);
}

void AssetVideoWidget::loadFile(const QString& filePath)
{
	const QString path = QDir::toNativeSeparators(filePath);

	// if path not empty try load it.
	if (path.isEmpty()) {
		showError("Empty path");
		return;
	}

	// do some stuff like validate extensions.
	// check it's valid etc..
	QFileInfo fi(path);
	if (!fi.exists())
	{
		showError("File does not exist");
		return;
	}
	else if (!fi.isFile())
	{
		showError("Path is not a file");
		return;
	}
	else if (!fileExtensionValid(path))
	{
		showError("Invalid file extension");
		return;
	}

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	if (!loadVideo(path)) {

	}

	QApplication::restoreOverrideCursor();
}


QString AssetVideoWidget::makeDialogTitle(const QString& title)
{
	if (dialogTitleOverride_.isNull()) {
		return title;
	}

	return dialogTitleOverride_;
}

bool AssetVideoWidget::fileExtensionValid(const QString& path)
{
	if (path.endsWith(".ivf", Qt::CaseInsensitive))
	{
		return true;
	}

	return false;
}


bool AssetVideoWidget::loadVideo(const QString& path)
{
	// so we want to load the image for both making a preview thumb.
	// and for saving to assetDB.
	QFileInfo fi(path);

	if (path.isEmpty() || !fi.exists()) {
		X_ERROR("Font", "Failed to load video, src file missing");
		return false;
	}

	if (!fileExtensionValid(path)) {
		X_ERROR("Font", "Failed to load video, invalid extension");
		return false;
	}

	// don't bother showing progress file files less than 512kb.
	bool showProgress = fi.size() > 1024 * 512;

	if (!pProgress_ && showProgress) {
		pProgress_ = new QProgressDialog("Importing..", "Cancel", 0, 30, ICore::mainWindow());
		pProgress_->setWindowModality(Qt::WindowModal);
		pProgress_->setCancelButton(nullptr);
		pProgress_->show();
	}

	loader_.loadFile(path);
	return true;
}

bool AssetVideoWidget::eventFilter(QObject* pObject, QEvent* pEvent)
{
	const auto type = pEvent->type();

	if (pObject == pDropZone_ || pObject == pLabel_)
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

void AssetVideoWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls())
	{
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

void AssetVideoWidget::dropEvent(QDropEvent* event)
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

				loadFile(filePath);

				event->accept();
			}
		}
	}
}

void AssetVideoWidget::showError(const QString& msg)
{
	QMessageBox::critical(ICore::mainWindow(), "Video", msg);
}

void AssetVideoWidget::browseClicked(void)
{
	QString predefined = ""; // path();

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

	QString newPath = QFileDialog::getOpenFileName(this, makeDialogTitle(tr("Choose Video File")),
		predefined, dialogFilter_);

	if (!newPath.isEmpty())
	{
		loadFile(newPath);
	}
}

void AssetVideoWidget::setProgress(int32_t pro)
{
	if (pProgress_) {
		pProgress_->setValue(pro);
	}
}

void AssetVideoWidget::setProgressLabel(const QString& label, int32_t pro)
{
	if (pProgress_) {
		pProgress_->setLabelText(label);
		pProgress_->setValue(pro);
	}
}

void AssetVideoWidget::rawFileLoaded(void)
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

	const auto srcDim = loader_.getSrcDim();
	const auto srcData = loader_.getCompressedSrc();

	pAssEntry_->updateRawFile(srcData);

	auto path = loader_.getPath().toStdString();

	setValue(path);

	emit valueChanged(path);
}

X_NAMESPACE_END