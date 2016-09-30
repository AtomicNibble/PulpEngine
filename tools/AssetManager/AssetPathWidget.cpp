#include "stdafx.h"
#include "AssetPathWidget.h"

X_NAMESPACE_BEGIN(assman)


AssetPathWidget::AssetPathWidget(QWidget *parent, const std::string& value) :
	QWidget(parent),
	pLineEdit_(nullptr),
	acceptingKind_(Kind::File)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);

	// lin edit for path.
	pLineEdit_ = new QLineEdit(this);
//	pLineEdit_->setAcceptDrops(true); // id have todo draw handeling diffrent to support just this accepting dorps.
	pLineEdit_->installEventFilter(this);
	connect(pLineEdit_, SIGNAL(editingFinished()), this, SLOT(editingFinished(void)));

	// browse button
	QToolButton* pBrowse = new QToolButton();
	pBrowse->setText("...");
	connect(pBrowse, SIGNAL(clicked()), this, SLOT(browseClicked()));

	pLayout->addWidget(pLineEdit_);
	pLayout->addWidget(pBrowse);

	setValue(value);

	setLayout(pLayout);
}

AssetPathWidget::~AssetPathWidget()
{
}

void AssetPathWidget::setExpectedKind(Kind expected)
{
	acceptingKind_ = expected;
}

AssetPathWidget::Kind AssetPathWidget::expectedKind(void) const
{
	return acceptingKind_;
}

void AssetPathWidget::setPromptDialogTitle(const QString& title)
{
	dialogTitleOverride_ = title;
}

QString AssetPathWidget::promptDialogTitle(void) const
{
	return dialogTitleOverride_;
}

void AssetPathWidget::setPromptDialogFilter(const QString& filter)
{
	dialogFilter_ = filter;
}

QString AssetPathWidget::promptDialogFilter(void) const
{
	return dialogFilter_;
}

void AssetPathWidget::setInitialBrowsePathBackup(const QString& path)
{
	initialBrowsePathOverride_ = path;
}

QString AssetPathWidget::baseDirectory(void) const
{
	return baseDirectory_;
}

void AssetPathWidget::setBaseDirectory(const QString& directory)
{
	baseDirectory_ = directory;
}

QString AssetPathWidget::baseFileName(void) const
{
	return baseDirectory_;
}

void AssetPathWidget::setBaseFileName(const QString& base)
{
	baseDirectory_ = base;
}

QString AssetPathWidget::path(void) const
{
	return expandedPath(QDir::fromNativeSeparators(pLineEdit_->text()));
}

QString AssetPathWidget::rawPath(void) const
{
	return QDir::fromNativeSeparators(pLineEdit_->text());
}

QString AssetPathWidget::fileName(void) const
{
	return path();
}

void AssetPathWidget::setPath(const QString& filePath)
{
	pLineEdit_->setText(QDir::toNativeSeparators(filePath));
}

void AssetPathWidget::setFileName(const QString& fileName)
{
	pLineEdit_->setText(QDir::toNativeSeparators(fileName));
}

QString AssetPathWidget::expandedPath(const QString& input) const
{
	if (input.isEmpty()) {
		return input;
	}

	const QString path = QDir::cleanPath(input);
	if (path.isEmpty()) {
		return path;
	}

	switch (acceptingKind_)
	{
	case Kind::Any:
		break;
	case Kind::Directory:
	case Kind::ExistingDirectory:
	case Kind::File:
	case Kind::SaveFile:
		if (!baseDirectory_.isEmpty() && QFileInfo(path).isRelative())
			return QFileInfo(baseDirectory_ + QLatin1Char('/') + path).absoluteFilePath();
		break;
	}
	return path;
}

QString AssetPathWidget::makeDialogTitle(const QString& title)
{
	if (dialogTitleOverride_.isNull()) {
		return title;
	}

	return dialogTitleOverride_;
}

bool AssetPathWidget::eventFilter(QObject* pObject, QEvent* pEvent)
{
	const auto type = pEvent->type();

	if (pObject == pLineEdit_)
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


void AssetPathWidget::dragEnterEvent(QDragEnterEvent *event)
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

void AssetPathWidget::dropEvent(QDropEvent* event)
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

				pLineEdit_->setText(filePath);
				
				event->accept();
			}
		}
	}
}

void AssetPathWidget::browseClicked(void)
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

	QString newPath;
	switch (acceptingKind_)
	{
	case Kind::Directory:
	case Kind::ExistingDirectory:
		newPath = QFileDialog::getExistingDirectory(this,
			makeDialogTitle(tr("Choose Directory")), predefined);
		break;

	case Kind::File: // fall through
		newPath = QFileDialog::getOpenFileName(this,
			makeDialogTitle(tr("Choose File")), predefined, dialogFilter_);
		break;
	case Kind::SaveFile:
		newPath = QFileDialog::getSaveFileName(this,
			makeDialogTitle(tr("Choose File")), predefined, dialogFilter_);
		break;
	case Kind::Any:
	{
		QFileDialog dialog(this);
		dialog.setFileMode(QFileDialog::AnyFile);
		dialog.setWindowTitle(makeDialogTitle(tr("Choose File")));

		//	if (fi.exists()) {
		//		dialog.setDirectory(fi.absolutePath());
		//	}

		dialog.setNameFilter(dialogFilter_);
		if (dialog.exec() == QDialog::Accepted) {

			QStringList paths = dialog.selectedFiles();
			if (!paths.isEmpty()) {
				newPath = paths.at(0);
			}
		}
		break;
	}

	default:
		break;
	}


	// Delete trailing slashes unless it is "/"|"\\", only
	if (!newPath.isEmpty())
	{
		newPath = QDir::toNativeSeparators(newPath);
		if (newPath.size() > 1 && newPath.endsWith(QDir::separator())) {
			newPath.truncate(newPath.size() - 1);
		}

		setPath(newPath);
	}

}

void AssetPathWidget::editingFinished(void)
{
	const QString value = pLineEdit_->text();

	emit valueChanged(value.toStdString());
}

void AssetPathWidget::setValue(const std::string& value)
{
	pLineEdit_->blockSignals(true);
	pLineEdit_->setText(QString::fromStdString(value));
	pLineEdit_->blockSignals(false);
}

X_NAMESPACE_END