#include "stdafx.h"
#include "VersionDialog.h"

X_NAMESPACE_BEGIN(editor)



VersionDialog::VersionDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(tr("About Editor"));
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	QGridLayout* pLayout = new QGridLayout(this);
	pLayout->setSizeConstraint(QLayout::SetFixedSize);

	const QString description = tr(
		"<h3>%1</h3>"
		"Site: tom-crowley.co.uk"
		"<br/>"
		"Built on %2 at %3<br />"
		"<br/>"
		"Copyright %4 %5. All rights reserved.<br/>"
		"<br/>")
		.arg(ICore::versionString(),
			QLatin1String(__DATE__), QLatin1String(__TIME__),
			QLatin1String(Constants::EDITOR_YEAR),
			QLatin1String(Constants::EDITOR_AUTHOR));


	QLabel* pCopyRightLabel = new QLabel(description);
	pCopyRightLabel->setWordWrap(true);
	pCopyRightLabel->setOpenExternalLinks(true);
	pCopyRightLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

	QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	QPushButton* pCloseButton = pButtonBox->button(QDialogButtonBox::Close);
	pButtonBox->addButton(pCloseButton, QDialogButtonBox::ButtonRole(QDialogButtonBox::RejectRole | QDialogButtonBox::AcceptRole));
	connect(pButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

	QLabel* pLogoLabel = new QLabel;
	pLogoLabel->setPixmap(QPixmap(QLatin1String(Constants::ICON_LOGO_128)));
	pLayout->addWidget(pLogoLabel, 0, 0, 1, 1);
	pLayout->addWidget(pCopyRightLabel, 0, 1, 4, 4);
	pLayout->addWidget(pButtonBox, 4, 0, 1, 5);
}


bool VersionDialog::event(QEvent *event)
{
	if (event->type() == QEvent::ShortcutOverride) {
		QKeyEvent *ke = static_cast<QKeyEvent *>(event);
		if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
			ke->accept();
			return true;
		}
	}
	return QDialog::event(event);
}

X_NAMESPACE_END