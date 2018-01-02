#include "stdafx.h"
#include "AssetTextWidget.h"

X_NAMESPACE_BEGIN(editor)

AssetTextWidget::AssetTextWidget(QWidget *parent, const std::string& value) :
	QWidget(parent)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);

	// lin edit for path.
	pTextEdit_ = new QPlainTextEdit();
	pTextEdit_->setAcceptDrops(true); // can drag file onto lineedit and it gets path.

	connect(pTextEdit_, SIGNAL(textChanged(void)), this, SLOT(textChanged(void)));

	pLayout->addWidget(pTextEdit_);

	setValue(value);

	setLayout(pLayout);
}

AssetTextWidget::~AssetTextWidget()
{
}


void AssetTextWidget::setValue(const std::string& value)
{
	pTextEdit_->blockSignals(true);
	{
		QString text = QString::fromStdString(value);
		text.replace("\\r\\n", "\n");

		pTextEdit_->setPlainText(text);
	}
	pTextEdit_->blockSignals(false);
}

void AssetTextWidget::textChanged(void)
{
	QString str = pTextEdit_->toPlainText();

	emit valueChanged(str.toStdString());
}

X_NAMESPACE_END