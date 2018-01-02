#include "stdafx.h"
#include "DisplayPixDialog.h"


X_NAMESPACE_BEGIN(assman)


DisplayPixDialog::DisplayPixDialog(QWidget *parent, QPixmap& pix)
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	// take me to the getto, where we can have some pickles
	QVBoxLayout* pLayout = new QVBoxLayout();

	QLabel* pImageLabel = new QLabel();
	pImageLabel->setFrameStyle(QFrame::NoFrame);
	pImageLabel->setAlignment(Qt::AlignCenter);

	if (pix.hasAlpha())
	{
		QPixmap alphaPix(pix.width(), pix.height());
		QPainter painter(&alphaPix);

		painter.setBackgroundMode(Qt::TransparentMode);
		painter.drawPixmap(0, 0, pix.width(), pix.height(), QPixmap(":/misc/img/checkerd.png"));
		painter.drawPixmap(0, 0, pix.width(), pix.height(), pix);
		painter.end();

		pImageLabel->setPixmap(alphaPix);
	}
	else
	{
		pImageLabel->setPixmap(pix);

	}

	QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	pButtonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(pButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(pButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

	pLayout->addWidget(pImageLabel);
	pLayout->addWidget(pButtonBox);

	setLayout(pLayout);
}

DisplayPixDialog::~DisplayPixDialog()
{
}

void DisplayPixDialog::accept(void)
{
	done(QDialog::Accepted);
}

void DisplayPixDialog::reject(void)
{
	done(QDialog::Rejected);
}


void DisplayPixDialog::done(int32_t val)
{
	QDialog::done(val);
}


X_NAMESPACE_END