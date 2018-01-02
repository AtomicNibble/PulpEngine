#include "stdafx.h"
#include "StatusBar.h"


X_NAMESPACE_BEGIN(editor)



MyStatusBar::MyStatusBar()
{
	Create();
}

MyStatusBar::MyStatusBar(QWidget* parent) : 
	QStatusBar(parent),
	pProgress_(nullptr)
{
	Create();
}


void MyStatusBar::Create()
{
	pProgress_ = new QProgressBar();
	pProgress_->setMaximumWidth(200);
	pProgress_->setRange(0, 0);
//	pProgress_->setValue(100);
	pProgress_->hide();

	addPermanentWidget(pProgress_, 0);
}

void MyStatusBar::showBusyBar(bool show)
{
	if (pProgress_->isVisible() == show) {
		return;
	}

	pProgress_->setVisible(show);

	if (show) {
		showMessage("Busy..");
	}
	else {
		showMessage("Ready");
	}


	setProperty("building", show);
	style()->polish(this);
}


X_NAMESPACE_END