#include "stdafx.h"
#include "AssetTextureWidget.h"

#include "DisplayPixDialog.h"

X_NAMESPACE_BEGIN(assman)

AssetTextureWidget::AssetTextureWidget(QWidget *parent, const std::string& value)
	: QWidget(parent),
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

		pEditPath_ = new QLineEdit(this);
		pEditPath_->setAcceptDrops(true); 
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

	showPreviewWidgets(false);

	setValue(value);

	setLayout(pLayout);
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

	showPreviewWidgets(false);
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
	setPath(QString::fromStdString(value));
	pEditPath_->blockSignals(false);
	blockSignals(false);
}

void AssetTextureWidget::editingFinished(void)
{
	setPath(pEditPath_->text());
}

void AssetTextureWidget::validatePath(void)
{
	const QString& curPath = curPath_;
	QString tip;

	if (!curPath.isEmpty())
	{
		// do some stuff like validate extensions.
		// check it's valid etc..
		QFileInfo fi(curPath);
		if (!fi.exists())
		{
			tip = "File does not exist";
		}
		else if (!fi.isFile())
		{
			tip = "Path is not a file";
		}
		else if (!fileExtensionValid(curPath_))
		{
			tip = "Invalid file extension";
		}
		else
		{
			if (pWarningAction_) {
				delete pWarningAction_;
			}

			// ok
			loadPreview();
			return;
		}

		if (!pWarningAction_)
		{
			QIcon icon(":/misc/img/warning_32.png");
			pWarningAction_ = pEditPath_->addAction(icon, QLineEdit::ActionPosition::LeadingPosition);
			pWarningAction_->setToolTip(tip);
		}
	}
}

void AssetTextureWidget::showPreviewWidgets(bool vis)
{
	pPreview_->setVisible(vis);
	pZoom_->setVisible(vis);
}

void AssetTextureWidget::loadPreview(void)
{
	QPixmap pix;

	if (!getPixMapCurPath(pix)) {
		showPreviewWidgets(false);
		return;
	}

	auto scaledPix = pix.scaled(64, 64);

	pPreview_->setPixmap(scaledPix);
	pEditDimensions_->setText(QString("%1x%2").arg(QString::number(pix.width()), QString::number(pix.height())));
	showPreviewWidgets(true);
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
	if (pix.width() > rect.width() || pix.height() > rect.height())
	{
		// pleb.. where you're 16k monitor at?
		pix = pix.scaled(rect.width() - 128, rect.width() - 128, Qt::KeepAspectRatio);
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