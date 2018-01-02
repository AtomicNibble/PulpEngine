#include "stdafx.h"
#include "AssetCheckBoxWidget.h"


X_NAMESPACE_BEGIN(editor)

AssetCheckBoxWidget::AssetCheckBoxWidget(QWidget *parent, const std::string& val) :
	QCheckBox(parent)
{

	connect(this, SIGNAL(toggled(bool)), this, SLOT(toggled(bool)));

	setValue(val);
}

AssetCheckBoxWidget::~AssetCheckBoxWidget()
{
}


void AssetCheckBoxWidget::setValue(const std::string& val)
{
	blockSignals(true);

	if (val.size() == 1 && val[0] == '1') {
		setChecked(true);
	}
	else {
		setChecked(false);
	}

	blockSignals(false);
}

void AssetCheckBoxWidget::toggled(bool checked)
{
	std::string valStd("0");
	if (checked) {
		valStd = "1";
	}

	emit valueChanged(valStd);
}

X_NAMESPACE_END