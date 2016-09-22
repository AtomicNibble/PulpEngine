#include "stdafx.h"
#include "AssetTextureWidget.h"

X_NAMESPACE_BEGIN(assman)

AssetTextureWidget::AssetTextureWidget(QWidget *parent)
	: QWidget(parent)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);

	// lin edit for path.
	QLineEdit* pEdit = new QLineEdit();
	pEdit->setAcceptDrops(true); // can drag file onto lineedit and it gets path.


								 // browse button
	QToolButton* pBrowse = new QToolButton();
	pBrowse->setText("...");


	connect(this, SIGNAL(clicked(bool)), pLayout, SLOT(browseClicked(bool)));


	pLayout->addWidget(pEdit);
	pLayout->addWidget(pBrowse);

	setLayout(pLayout);
}

AssetTextureWidget::~AssetTextureWidget()
{
}


void AssetTextureWidget::browseClicked(void)
{

}


X_NAMESPACE_END