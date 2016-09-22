#include "stdafx.h"
#include "AssetTextWidget.h"

X_NAMESPACE_BEGIN(assman)

AssetTextWidget::AssetTextWidget(QWidget *parent, const std::string& value)
	: QWidget(parent)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);

	// lin edit for path.
	QPlainTextEdit* pEdit = new QPlainTextEdit();
	pEdit->setAcceptDrops(true); // can drag file onto lineedit and it gets path.

	connect(this, SIGNAL(textChanged(bool)), pEdit, SLOT(textChanged()));

	pLayout->addWidget(pEdit);

	pEdit->blockSignals(true);
	{
		QString text = QString::fromStdString(value);
		text.replace("\\r\\n", "\n");

		pEdit->setPlainText(text);
	}
	pEdit->blockSignals(false);

	setLayout(pLayout);
}

AssetTextWidget::~AssetTextWidget()
{
}

void AssetTextWidget::textChanged(void)
{


}

X_NAMESPACE_END