#include "stdafx.h"
#include "AssetTextureWidget.h"

#include "DisplayPixDialog.h"

#include "IAssetEntry.h"

#include <String\HumanSize.h>
#include <Compression\LZ4.h>
#include <Time\StopWatch.h>

X_NAMESPACE_BEGIN(assman)

AssetTextureWidget::AssetTextureWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value) :
	QWidget(parent),
	pAssEntry_(pAssEntry),
	pWarningAction_(nullptr)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);

	pPreview_ = new QLabel();
	pPreview_->installEventFilter(this);

	pZoom_ = new QToolButton();
	pZoom_->setIcon(QIcon(":/misc/img/Zoom.png"));
	pZoom_->setToolTip("View image");
	connect(pZoom_, SIGNAL(clicked()), this, SLOT(showTextureDialog()));

	QVBoxLayout* pVertLayout = new QVBoxLayout();
	pVertLayout->setContentsMargins(0, 0, 0, 0);
	{
		pEditPathPad_ = new QLabel();

		pEditPath_ = new QLineEdit();
		pEditPath_->installEventFilter(this);

		connect(pEditPath_, SIGNAL(editingFinished()), this, SLOT(editingFinished()));

		pEditDimensions_ = new QLabel();

		pVertLayout->addWidget(pEditPathPad_);
		pVertLayout->addWidget(pEditPath_);
		pVertLayout->addWidget(pEditDimensions_);
	}

	// browse button
	QToolButton* pBrowse = new QToolButton();
	pBrowse->setText("...");
	setPromptDialogFilter("DDS (*.dds);;PNG (*.png);;TGA (*.tga);;JPG(*.jpg)");

	connect(pBrowse, SIGNAL(clicked()), this, SLOT(browseClicked()));

	pLayout->addWidget(pPreview_);
	pLayout->addWidget(pZoom_);
	pLayout->addLayout(pVertLayout);
	pLayout->addWidget(pBrowse);

	setLayout(pLayout);

	setValue(value);
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

QString AssetTextureWidget::path(void) const
{
	return curPath_;
}

void AssetTextureWidget::setPath(const QString& filePath)
{
	if (filePath == curPath_) {
		return;
	} 

	// always clear dims i think.
	// leave previews they get hidden in validatePath.
	pEditDimensions_->setText("");

	curPath_ = QDir::toNativeSeparators(filePath);
	pEditPath_->setText(curPath_);

	validatePath();

	emit valueChanged(curPath_.toStdString());
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

bool AssetTextureWidget::getPixMapCurPath(QPixmap& pixOut)
{
	const QString& curPath = curPath_;
	QFileInfo fi(curPath);

	if (curPath.isEmpty() || !fi.exists()) {
		return false;
	}

	if (fileExtensionValid(curPath))
	{
		if (pixOut.load(curPath)) {
			return true;
		}
	}

	// failed to load.
	return false;
}

bool AssetTextureWidget::loadImage(void)
{
	// so we want to load the image for both making a preview thumb.
	// and for saving to assetDB.
	const QString& curPath = curPath_;
	QFileInfo fi(curPath);

	if (curPath.isEmpty() || !fi.exists()) {
		X_ERROR("Img", "Failed to load image, src file missing");
		return false;
	}

	if (!fileExtensionValid(curPath)) {
		X_ERROR("Img", "Failed to load image, invalid extension");
		return false;
	}

	QFile f(curPath_);
	if (!f.open(QFile::ReadOnly)) {
		X_ERROR("Img", "Failed to load image, error opening file. Err: %s", qPrintable(f.errorString()));
		return false;
	}

	const auto fileSize = f.size();

	core::Array<uint8_t> imgData(g_arena);
	imgData.resize(fileSize);
	if (f.read(reinterpret_cast<char*>(imgData.data()), imgData.size()) != fileSize) {
		X_ERROR("Img", "Error reading img file data");
		return false;
	}

	core::Array<uint8_t> compressed(g_arena);
	core::Compression::Compressor<core::Compression::LZ4> comp;

	{
		core::StopWatch timer;

		if (!comp.deflate(g_arena, imgData, compressed, core::Compression::CompressLevel::NORMAL))
		{
			X_ERROR("Img", "Failed to defalte img src");
			return false;
		}
		else
		{
			const auto elapsed = timer.GetMilliSeconds();
			const float percentageSize = (static_cast<float>(compressed.size()) / static_cast<float>(imgData.size())) * 100;

			core::HumanSize::Str sizeStr, sizeStr2;
			X_LOG0("Img", "Defalated src img %s -> %s(%.2g%%) %gms",
				core::HumanSize::toString(sizeStr, imgData.size()),
				core::HumanSize::toString(sizeStr2, compressed.size()),
				percentageSize,
				elapsed);
		}

	}

	pAssEntry_->updateRawFile(compressed);

	// preview creation
	QPixmap pix;

	if (!pix.loadFromData(imgData.data(), static_cast<uint32_t>(imgData.size()))) {
		X_ERROR("Img", "Error creating pixmap for preview");
		return false;
	}

	auto scaledPix = pix.scaled(64, 64);

	// extract scaled image.
	{
		QByteArray bytes;
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::WriteOnly);
		if (!scaledPix.save(&buffer, "BMP")) {
			X_ERROR("Texture", "Failed to save thumb");
		}
		else
		{
			core::Array<uint8_t> data(g_arena, bytes.count());
			std::memcpy(data.data(), bytes.constData(), data.size());

			pAssEntry_->updateThumb(data, Vec2i(64,64), Vec2i(pix.width(), pix.height()));
		}
	}

	// set and show dims
	pEditDimensions_->setText(QString("%1x%2").arg(QString::number(pix.width()), QString::number(pix.height())));

	// set preivew pix
	pPreview_->setPixmap(scaledPix);
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

void AssetTextureWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls()) {
		auto urls = event->mimeData()->urls();
		if (urls.size() == 1)
		{
			const auto& url = urls[0];
			if (url.isLocalFile())
			{
				event->setDropAction(Qt::DropAction::LinkAction);
				event->accept();
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

				setPath(filePath);

				event->accept();
			}
		}
	}
}

void AssetTextureWidget::setValue(const std::string& value)
{
	blockSignals(true);
	pEditPath_->blockSignals(true);

	// can't use set path as it will try load the image.
	// what we want todo instead is get preview.
	// this wil be the path of the saved file.
	// put what if we revert so preivew actually changes.
	// technically you can't revert a rawfile update.
	// as it's saved.
	// for now just get preview.
	if (!value.empty())
	{
		core::Array<uint8_t> thumbData(g_arena);
		Vec2i dim;

		if (pAssEntry_->getThumb(thumbData, dim))
		{
			QPixmap pix;

			pix.loadFromData(thumbData.data(), static_cast<int32_t>(thumbData.size()));

			pPreview_->setPixmap(pix);
			pEditDimensions_->setText(QString("%1x%2").arg(QString::number(dim.x), QString::number(dim.y)));
			showPreviewWidgets(true);
		}
	}

	curPath_ = QDir::toNativeSeparators(QString::fromStdString(value));
	pEditPath_->setText(curPath_);

	pEditPath_->blockSignals(false);
	blockSignals(false);
}

void AssetTextureWidget::editingFinished(void)
{
	setPath(pEditPath_->text());
}


void AssetTextureWidget::setErrorTip(const QString& tip)
{
	showPreviewWidgets(false);

	if (!pWarningAction_) {
		QIcon icon(":/misc/img/warning_32.png");
		pWarningAction_ = pEditPath_->addAction(icon, QLineEdit::ActionPosition::LeadingPosition);
		pWarningAction_->setToolTip(tip);
	}
}

void AssetTextureWidget::removeErrorTip(void)
{
	if (pWarningAction_) {
		delete pWarningAction_;
		pWarningAction_ = nullptr;
	}
}

void AssetTextureWidget::validatePath(void)
{
	const QString& curPath = curPath_;

	// if path not empty try load it.
	if (curPath.isEmpty()){
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

	// file exsists and has correct extension.
	removeErrorTip();

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	if (!loadImage()) {
		showPreviewWidgets(false);
	}
	else {
		showPreviewWidgets(true);
	}

	QApplication::restoreOverrideCursor();
}

void AssetTextureWidget::showPreviewWidgets(bool vis)
{
	pPreview_->setVisible(vis);
	pZoom_->setVisible(vis);
}

void AssetTextureWidget::showTextureDialog(void)
{
	QPixmap pix;

	if (!getPixMapCurPath(pix)) {
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

	QFileInfo fi(path());

	DisplayPixDialog dialog(this, pix);
	dialog.setWindowTitle(QString("%1 - %2x%3").arg(fi.baseName(), QString::number(imgSize.width()), QString::number(imgSize.height())));
	dialog.exec();
}

void AssetTextureWidget::browseClicked(void)
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

	QString newPath = QFileDialog::getOpenFileName(this, makeDialogTitle(tr("Choose Texture File")), 
		predefined, dialogFilter_);

	if (!newPath.isEmpty())
	{
		setPath(newPath);
	}
}



X_NAMESPACE_END