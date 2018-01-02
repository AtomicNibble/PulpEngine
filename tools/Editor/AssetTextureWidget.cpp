#include "stdafx.h"
#include "AssetTextureWidget.h"

#include "DisplayPixDialog.h"

#include "IAssetEntry.h"

#include <String\HumanSize.h>
#include <Compression\LZ4.h>
#include <Time\StopWatch.h>

X_NAMESPACE_BEGIN(assman)


// ----------------------------------


AssetTextureWidget::AssetTextureWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value) :
	QWidget(parent),
	pAssEntry_(pAssEntry),
	pProgress_(nullptr),
	loader_(g_arena, assetDb::AssetType::IMG)
{
	X_UNUSED(value);

	pPreview_ = new QLabel();
	pPreview_->installEventFilter(this);

	pZoom_ = new QToolButton();
	pZoom_->setIcon(QIcon(":/misc/img/Zoom.png"));
	pZoom_->setToolTip("View image");
	connect(pZoom_, SIGNAL(clicked()), this, SLOT(showTextureDialog()));

	QPixmap imgPix(":/assetDb/img/Drop_img.png");
	pDropZone_ = new QLabel();
	pDropZone_->setPixmap(imgPix);
	pDropZone_->installEventFilter(this);
	pDropZone_->setAcceptDrops(true);

	pEditDimensions_ = new QLabel();
	pEditDimensions_->setAlignment(Qt::AlignCenter);

	// browse button
	QToolButton* pBrowse = new QToolButton();
	pBrowse->setText("...");
	setPromptDialogFilter("Texture (*.dds *.png *.tga *.jpg);;DDS (*.dds);;PNG (*.png);;TGA (*.tga);;JPG (*.jpg)");

	connect(pBrowse, SIGNAL(clicked()), this, SLOT(browseClicked()));

	//  P | hoz
 	//  P
	QGridLayout* pLayout = new QGridLayout();
	{
		QVBoxLayout* pVertLayout = new QVBoxLayout();
		pVertLayout->setContentsMargins(0, 0, 0, 0);
		pVertLayout->addWidget(pPreview_, 1);
		pVertLayout->addWidget(pEditDimensions_);

		QHBoxLayout* pHozLayout = new QHBoxLayout();
		pHozLayout->setContentsMargins(0, 0, 0, 0);
		pHozLayout->addWidget(pZoom_);
		pHozLayout->addWidget(pDropZone_, 2);
		pHozLayout->addWidget(pBrowse);

		pLayout->addLayout(pVertLayout, 0,0, 2, 1);
		pLayout->addLayout(pHozLayout, 0, 1, 1, 1);
	}

	setLayout(pLayout);

	connect(&loader_, &RawFileLoader::setProgress, this, &AssetTextureWidget::setProgress, Qt::QueuedConnection);
	connect(&loader_, &RawFileLoader::setProgressLabel, this, &AssetTextureWidget::setProgressLabel, Qt::QueuedConnection);
	connect(&loader_, SIGNAL(finished()), this, SLOT(rawFileLoaded()));

	loadPreview();
}

AssetTextureWidget::~AssetTextureWidget()
{
}


void AssetTextureWidget::setPromptDialogTitle(const QString& title)
{
	dialogTitleOverride_ = title;
}

QString AssetTextureWidget::promptDialogTitle(void) const
{
	return dialogTitleOverride_;
}

void AssetTextureWidget::setPromptDialogFilter(const QString& filter)
{
	dialogFilter_ = filter;
}

QString AssetTextureWidget::promptDialogFilter(void) const
{
	return dialogFilter_;
}

void AssetTextureWidget::loadFile(const QString& filePath)
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

	if (!loadImage(path)) {
		showPreviewWidgets(false);
	}
	else {
		showPreviewWidgets(true);
	}

	QApplication::restoreOverrideCursor();
}


QString AssetTextureWidget::makeDialogTitle(const QString& title)
{
	if (dialogTitleOverride_.isNull()) {
		return title;
	}

	return dialogTitleOverride_;
}

bool AssetTextureWidget::fileExtensionValid(const QString& path)
{
	if (path.endsWith(".png", Qt::CaseInsensitive) ||
		path.endsWith(".jpg", Qt::CaseInsensitive) ||
		path.endsWith(".tga", Qt::CaseInsensitive) ||
		path.endsWith(".dds", Qt::CaseInsensitive))
	{
		return true;
	}
	
	return false;
}

bool AssetTextureWidget::loadCurrentToPix(QPixmap& pixOut)
{
	core::Array<uint8_t> rawData(g_arena);

	if (pAssEntry_->getRawFile(rawData))
	{
		if (pixOut.loadFromData(rawData.data(), safe_static_cast<uint32_t>(rawData.size()))) {
			return true;
		}
		
		// tga needs a helping hand.
		if (pixOut.loadFromData(rawData.data(), safe_static_cast<uint32_t>(rawData.size()), "tga")) {
			return true;
		}

		showError("Failed to create preview from raw data");
	}

	return false;
}

void AssetTextureWidget::setPreviewPix(QPixmap& pix)
{
	if (pix.hasAlpha())
	{
		QPixmap alphaPix(pix.width(), pix.height());
		QPainter painter(&alphaPix);

		painter.setBackgroundMode(Qt::TransparentMode);
		painter.drawPixmap(0, 0, pix.width(), pix.height(), QPixmap(":/misc/img/checkerd.png"));
		painter.drawPixmap(0, 0, pix.width(), pix.height(), pix);
		painter.end();

		pPreview_->setPixmap(alphaPix);
	}
	else
	{
		pPreview_->setPixmap(pix);
	}
}

bool AssetTextureWidget::loadImage(const QString& path)
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

bool AssetTextureWidget::eventFilter(QObject* pObject, QEvent* pEvent)
{
	const auto type = pEvent->type();

	if (type == QEvent::MouseButtonRelease && pPreview_ == pObject)
	{
		QMouseEvent* pMouseEvent = static_cast<QMouseEvent*>(pEvent);
		const auto pos = pMouseEvent->pos();

		if (pPreview_->rect().contains(pos))
		{
			// show pre view.
			showTextureDialog();
		}
	}

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

void AssetTextureWidget::dragEnterEvent(QDragEnterEvent *event)
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

void AssetTextureWidget::dropEvent(QDropEvent* event)
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

void AssetTextureWidget::showError(const QString& msg)
{
	QMessageBox::critical(ICore::mainWindow(), "Image", msg);
}



void AssetTextureWidget::loadPreview(void)
{
	core::Array<uint8_t> thumbData(g_arena);
	Vec2i dim;

	if (pAssEntry_->getThumb(thumbData, dim))
	{
		QPixmap pix;

		pix.loadFromData(thumbData.data(), static_cast<int32_t>(thumbData.size()));

		setPreviewPix(pix);

		pEditDimensions_->setText(QString("%1x%2").arg(QString::number(dim.x), QString::number(dim.y)));
		showPreviewWidgets(true);
	}
}


void AssetTextureWidget::showPreviewWidgets(bool vis)
{
	pPreview_->setVisible(vis);
	pZoom_->setVisible(vis);
}

void AssetTextureWidget::showTextureDialog(void)
{
	QPixmap pix;

	if (!loadCurrentToPix(pix)) {
		return;
	}

	const auto imgSize = pix.rect();

	// make sure it fits on the fucking screen twat!
	const auto rect = QApplication::desktop()->screenGeometry();
	if (pix.width() > (rect.width() - 96) || pix.height() > (rect.height() - 96))
	{
		// pleb.. where you're 16k monitor at?
		const auto scaledWidth = core::Max(64, rect.width() - 128);
		const auto scaledHeight = core::Max(64, rect.height() - 192);

		pix = pix.scaled(scaledWidth, scaledHeight, Qt::KeepAspectRatio);
	}


	DisplayPixDialog dialog(this, pix);
	dialog.setWindowTitle(QString("Raw Preview - %2x%3").arg(QString::number(imgSize.width()), QString::number(imgSize.height())));
	dialog.exec();
}

void AssetTextureWidget::browseClicked(void)
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

	QString newPath = QFileDialog::getOpenFileName(this, makeDialogTitle(tr("Choose Texture File")), 
		predefined, dialogFilter_);

	if (!newPath.isEmpty())
	{
		loadFile(newPath);
	}
}

void AssetTextureWidget::setProgress(int32_t pro)
{
	if (pProgress_) {
		pProgress_->setValue(pro);
	}
}

void AssetTextureWidget::setProgressLabel(const QString& label, int32_t pro)
{
	if (pProgress_) {
		pProgress_->setLabelText(label);
		pProgress_->setValue(pro);
	}
}

void AssetTextureWidget::rawFileLoaded(void)
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
	const auto thumbData = loader_.getThumbData();

	// src image data
	pAssEntry_->updateRawFile(srcData);

	// thumb data
	if (thumbData.isNotEmpty()) {
		pAssEntry_->updateThumb(thumbData, Vec2i(64, 64), srcDim,
			core::Compression::Algo::STORE, core::Compression::CompressLevel::NORMAL);
	}

	// set and show dims
	pEditDimensions_->setText(QString("%1x%2").arg(QString::number(srcDim.x), QString::number(srcDim.y)));

	// set preivew pix
	if (thumbData.isNotEmpty()) {
		QPixmap thumbPix;
		thumbPix.loadFromData(thumbData.data(), static_cast<int32_t>(thumbData.size()));
		setPreviewPix(thumbPix);
	}
}

X_NAMESPACE_END