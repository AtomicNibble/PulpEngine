#include "stdafx.h"
#include "AssetTextWidget.h"

X_NAMESPACE_BEGIN(assman)

AssetTextWidget::AssetTextWidget(QWidget *parent, const std::string& value)
	: QWidget(parent)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);

	// lin edit for path.
	pTextEdit_ = new QPlainTextEdit();
	pTextEdit_->setAcceptDrops(true); // can drag file onto lineedit and it gets path.

	connect(this, SIGNAL(textChanged(bool)), pTextEdit_, SLOT(textChanged()));

	pLayout->addWidget(pTextEdit_);

	pTextEdit_->blockSignals(true);
	{
		QString text = QString::fromStdString(value);
		text.replace("\\r\\n", "\n");

		pTextEdit_->setPlainText(text);
	}
	pTextEdit_->blockSignals(false);

	setLayout(pLayout);
}

AssetTextWidget::~AssetTextWidget()
{
}

void AssetTextWidget::textChanged(void)
{

}

X_NAMESPACE_END