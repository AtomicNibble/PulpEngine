#include "stdafx.h"
#include "AssetCheckBoxWidget.h"

X_NAMESPACE_BEGIN(assman)

AssetCheckBoxWidget::AssetCheckBoxWidget(QWidget *parent, const std::string& val)
	: QCheckBox(parent)
{

	connect(this, SIGNAL(toggled(bool)), this, SLOT(toggled(bool)));

	blockSignals(true);
	
	if (val.size() == 1 && val[0] == '1') {
		setChecked(true);
	}
	else {
		setChecked(false);
	}

	blockSignals(false);
}

AssetCheckBoxWidget::~AssetCheckBoxWidget()
{
}


void AssetCheckBoxWidget::toggled(bool checked)
{
	X_UNUSED(checked);



}

X_NAMESPACE_END