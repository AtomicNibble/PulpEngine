#include "stdafx.h"
#include "AssetSpinBoxWidget.h"


X_NAMESPACE_BEGIN(assman)


AssetDoubleSpinBoxWidget::AssetDoubleSpinBoxWidget(QWidget *parent, double val, double min, double max, double step)
	: QWidget(parent)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);

	// lin edit for path.
	QDoubleSpinBox* pSpinBox = new QDoubleSpinBox();
	pSpinBox->setValue(val);
	pSpinBox->setRange(min, max);
	pSpinBox->setSingleStep(step);
	pSpinBox->setMinimumWidth(96);

	pLayout->addWidget(pSpinBox);
	pLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));

	connect(this, SIGNAL(valueChanged(double)), pLayout, SLOT(valueChanged(double)));

	setLayout(pLayout);
}

AssetDoubleSpinBoxWidget::~AssetDoubleSpinBoxWidget()
{
}


void AssetDoubleSpinBoxWidget::valueChanged(double val)
{
	X_UNUSED(val);


}



// -------------------------------------------------


AssetSpinBoxWidget::AssetSpinBoxWidget(QWidget *parent, int32_t val, double min, double max, double step)
	: QWidget(parent)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);

	// lin edit for path.
	QSpinBox* pSpinBox = new QSpinBox();
	pSpinBox->setValue(val);
	pSpinBox->setRange(static_cast<int32_t>(math<double>::floor(min)), static_cast<int32_t>(math<double>::floor(max)));
	pSpinBox->setSingleStep(static_cast<int32_t>(math<double>::floor(step)));
	pSpinBox->setMinimumWidth(96);

	pLayout->addWidget(pSpinBox);
	pLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));

	connect(this, SIGNAL(valueChanged(int)), pLayout, SLOT(valueChanged(int)));

	setLayout(pLayout);
}

AssetSpinBoxWidget::~AssetSpinBoxWidget()
{
}


void AssetSpinBoxWidget::valueChanged(int32_t val)
{
	X_UNUSED(val);

}


X_NAMESPACE_END