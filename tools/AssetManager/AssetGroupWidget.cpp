#include "stdafx.h"
#include "AssetGroupWidget.h"


X_NAMESPACE_BEGIN(assman)

AssetGroupWidget::AssetGroupWidget(QWidget *parent)
	: QToolButton(parent)
{

	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setAutoRaise(true);
	setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
//	setText(QString::fromStdString(title_));

	auto f = font();
	f.setBold(true);
	setFont(f);

	// font.setPixelSize(;)
	{
		setIcon(QIcon(":/misc/img/collapse.png"));
		setIconSize(QSize(12, 12));
	}


	connect(this, SIGNAL(clicked(bool)), this, SLOT(buttonClicked(bool)));

}

AssetGroupWidget::~AssetGroupWidget()
{
}


void AssetGroupWidget::buttonClicked(bool )
{

	int goat = 0;
	goat = 1;

	this->setVisible(false);
}

X_NAMESPACE_END