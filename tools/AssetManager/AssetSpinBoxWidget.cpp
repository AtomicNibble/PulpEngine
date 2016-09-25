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
	pSpinBox->setRange(min, max);
	pSpinBox->setSingleStep(step);
	pSpinBox->setMinimumWidth(96);

	connect(pSpinBox, SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)));

	pSpinBox->blockSignals(true);
	pSpinBox->setValue(val);
	pSpinBox->blockSignals(false);

	pLayout->addWidget(pSpinBox);
	pLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));


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
	pSpinBox->setRange(static_cast<int32_t>(math<double>::floor(min)), static_cast<int32_t>(math<double>::floor(max)));
	pSpinBox->setSingleStep(static_cast<int32_t>(math<double>::floor(step)));
	pSpinBox->setMinimumWidth(96);

	pSpinBox->blockSignals(true);
	pSpinBox->setValue(val);
	pSpinBox->blockSignals(false);


	connect(pSpinBox, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));

	pLayout->addWidget(pSpinBox);
	pLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));

	setLayout(pLayout);
}

AssetSpinBoxWidget::~AssetSpinBoxWidget()
{
}


void AssetSpinBoxWidget::valueChanged(int val)
{
	X_UNUSED(val);

}


X_NAMESPACE_END