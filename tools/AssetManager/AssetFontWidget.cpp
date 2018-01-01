#include "stdafx.h"
#include "AssetFontWidget.h"

#include "DisplayPixDialog.h"

#include "IAssetEntry.h"

#include <String\HumanSize.h>
#include <Compression\LZ4.h>
#include <Time\StopWatch.h>

X_NAMESPACE_BEGIN(assman)


// ----------------------------------


AssetFontWidget::AssetFontWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value) :
	QWidget(parent),
	pAssEntry_(pAssEntry),
	pProgress_(nullptr),
	loader_(g_arena, assetDb::AssetType::FONT)
{
	X_UNUSED(value);

	QPixmap imgPix(":/assetDb/img/Drop_font.png");
	pDropZone_ = new QLabel();
	pDropZone_->setPixmap(imgPix);
	pDropZone_->installEventFilter(this);
	pDropZone_->setAcceptDrops(true);

	// browse button
	QToolButton* pBrowse = new QToolButton();
	pBrowse->setText("...");
	setPromptDialogFilter("Font (*.ttf);;TrueType (*.ttf)");

	connect(pBrowse, SIGNAL(clicked()), this, SLOT(browseClicked()));

	//  P | hoz
	//  P
	QGridLayout* pLayout = new QGridLayout();
	{
		QVBoxLayout* pVertLayout = new QVBoxLayout();
		pVertLayout->setContentsMargins(0, 0, 0, 0);
		
		QHBoxLayout* pHozLayout = new QHBoxLayout();
		pHozLayout->setContentsMargins(0, 0, 0, 0);
		pHozLayout->addWidget(pDropZone_, 2);
		pHozLayout->addWidget(pBrowse);

		pLayout->addLayout(pVertLayout, 0, 0, 2, 1);
		pLayout->addLayout(pHozLayout, 0, 1, 1, 1);
	}

	setLayout(pLayout);

	connect(&loader_, &RawFileLoader::setProgress, this, &AssetFontWidget::setProgress, Qt::QueuedConnection);
	connect(&loader_, &RawFileLoader::setProgressLabel, this, &AssetFontWidget::setProgressLabel, Qt::QueuedConnection);
	connect(&loader_, SIGNAL(finished()), this, SLOT(rawFileLoaded()));
}

AssetFontWidget::~AssetFontWidget()
{
}


void AssetFontWidget::setPromptDialogTitle(const QString& title)
{
	dialogTitleOverride_ = title;
}

QString AssetFontWidget::promptDialogTitle(void) const
{
	return dialogTitleOverride_;
}

void AssetFontWidget::setPromptDialogFilter(const QString& filter)
{
	dialogFilter_ = filter;
}

QString AssetFontWidget::promptDialogFilter(void) const
{
	return dialogFilter_;
}

void AssetFontWidget::loadFile(const QString& filePath)
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

	if (!loadFont(path)) {
	
	}

	QApplication::restoreOverrideCursor();
}


QString AssetFontWidget::makeDialogTitle(const QString& title)
{
	if (dialogTitleOverride_.isNull()) {
		return title;
	}

	return dialogTitleOverride_;
}

bool AssetFontWidget::fileExtensionValid(const QString& path)
{
	if (path.endsWith(".ttf", Qt::CaseInsensitive))
	{
		return true;
	}

	return false;
}


bool AssetFontWidget::loadFont(const QString& path)
{
	// so we want to load the image for both making a preview thumb.
	// and for saving to assetDB.
	QFileInfo fi(path);

	if (path.isEmpty() || !fi.exists()) {
		X_ERROR("Img", "Failed to load image, src file missing");
		return false;
	}

	if (!fileExtensionValid(path)) {
		X_ERROR("Img", "Failed to load image, invalid extension");
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

bool AssetFontWidget::eventFilter(QObject* pObject, QEvent* pEvent)
{
	const auto type = pEvent->type();

	if (pObject == pDropZone_)
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

void AssetFontWidget::dragEnterEvent(QDragEnterEvent *event)
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

void AssetFontWidget::dropEvent(QDropEvent* event)
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

void AssetFontWidget::showError(const QString& msg)
{
	QMessageBox::critical(ICore::mainWindow(), "Image", msg);
}

void AssetFontWidget::browseClicked(void)
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

	QString newPath = QFileDialog::getOpenFileName(this, makeDialogTitle(tr("Choose Font File")),
		predefined, dialogFilter_);

	if (!newPath.isEmpty())
	{
		loadFile(newPath);
	}
}

void AssetFontWidget::setProgress(int32_t pro)
{
	if (pProgress_) {
		pProgress_->setValue(pro);
	}
}

void AssetFontWidget::setProgressLabel(const QString& label, int32_t pro)
{
	if (pProgress_) {
		pProgress_->setLabelText(label);
		pProgress_->setValue(pro);
	}
}

void AssetFontWidget::rawFileLoaded(void)
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

	// src image data
	pAssEntry_->updateRawFile(srcData);
}

X_NAMESPACE_END