#include "stdafx.h"
#include "AssetGroupWidget.h"


X_NAMESPACE_BEGIN(assman)

AssetGroupWidget::AssetGroupWidget(QWidget *parent)
	: QToolButton(parent)
{

	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setAutoRaise(true);
	setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
	setCheckable(true);

	auto f = font();
	f.setBold(true);
	setFont(f);

	{
		QPixmap collapsePix(":/misc/img/collapse.png");
		QPixmap expandPix(":/misc/img/expand.png");

		QIcon icon;
		icon.addPixmap(collapsePix, QIcon::Normal, QIcon::Off);
		icon.addPixmap(expandPix, QIcon::Normal, QIcon::On);

		setIcon(icon);
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

//	this->setVisible(false);
}

X_NAMESPACE_END