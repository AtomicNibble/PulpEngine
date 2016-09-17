#include "stdafx.h"
#include "BaseWindow.h"

#include <QStatusBar>


X_NAMESPACE_BEGIN(assman)


BaseWindow::BaseWindow(QWidget * parent) :
	QWidget(parent),
	mainLayout_(this),
	centralWidget_(nullptr)
{
	setMinimumSize(128, 64);
//	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
	setObjectName("BaseWindow");

	mainLayout_.setMargin(0);   //  No  space  between  window's  element  and  the  border
	mainLayout_.setSpacing(0);  //  No  space  between  window's  element
	setLayout(&mainLayout_);


	// left/right spacer
	mainLayout_.addItem(new QSpacerItem(borderWidthGUI, borderWidthGUI), 1, 0, 1, 1);
	mainLayout_.addItem(new QSpacerItem(borderWidthGUI, borderWidthGUI), 1, 2, 1, 1);
	// bottom space
	mainLayout_.addItem(new QSpacerItem(borderHeightGUI, borderHeightGUI), 2, 0, 1, 3);

	mainLayout_.setRowStretch(1, 1); //  Put  the  title  bar  at  the  top  of  the  window


}


BaseWindow::~BaseWindow()
{

}


void BaseWindow::setWindowTitle(const QString& title)
{
	QWidget::setWindowTitle(title);

	emit WindowTitleChanged();
}


void BaseWindow::setCentralWidget(QWidget* widget)
{
	mainLayout_.addWidget(widget, 1, 1, 1, 1);
	centralWidget_ = widget;
}

void BaseWindow::setCentralWidget(QLayout* layout)
{
	mainLayout_.addLayout(layout, 1, 1, 1, 1);
}

void BaseWindow::setStatusBar(QStatusBar* statusbar)
{
	mainLayout_.addWidget(statusbar, 3, 0, 1, 3);
}

void BaseWindow::setMainLayoutName(const QString& name)
{
	mainLayout_.setObjectName(name);
}


void BaseWindow::raiseWindow(void)
{
	setWindowState(windowState() & ~Qt::WindowMinimized);

	raise();

	activateWindow();
}



X_NAMESPACE_END