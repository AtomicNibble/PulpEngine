#include "stdafx.h"
#include "AssetLineEditWidget.h"

X_NAMESPACE_BEGIN(assman)

AssetLineEditWidget::AssetLineEditWidget(QWidget *parent, const std::string& value) :
	QLineEdit(parent)
{
	connect(this, SIGNAL(editingFinished()), this, SLOT(editingFinished(void)));

	setValue(value);
}

AssetLineEditWidget::~AssetLineEditWidget()
{
}

void AssetLineEditWidget::setValue(const std::string& value)
{
	blockSignals(true);
	setText(QString::fromStdString(value));
	blockSignals(false);
}


void AssetLineEditWidget::editingFinished(void)
{
	const QString value = text();

	emit valueChanged(value.toStdString());
}


X_NAMESPACE_END