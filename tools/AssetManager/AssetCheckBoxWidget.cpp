#include "stdafx.h"
#include "AssetCheckBoxWidget.h"

X_NAMESPACE_BEGIN(assman)

AssetCheckBoxWidget::AssetCheckBoxWidget(QWidget *parent, const std::string& val)
	: QCheckBox(parent)
{

	connect(this, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));

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


void AssetCheckBoxWidget::stateChanged(int32_t state)
{
	X_UNUSED(state);



}

X_NAMESPACE_END