#include "stdafx.h"
#include "AssetPathWidget.h"

X_NAMESPACE_BEGIN(assman)


AssetPathWidget::AssetPathWidget(QWidget *parent, const std::string& value)
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

	pEdit->blockSignals(true);
	pEdit->setText(QString::fromStdString(value));
	pEdit->blockSignals(false);

	setLayout(pLayout);
}

AssetPathWidget::~AssetPathWidget()
{
}


void AssetPathWidget::browseClicked(void)
{


}

X_NAMESPACE_END