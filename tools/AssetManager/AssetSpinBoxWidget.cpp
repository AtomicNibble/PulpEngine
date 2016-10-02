#include "stdafx.h"
#include "AssetSpinBoxWidget.h"


X_NAMESPACE_BEGIN(assman)


AssetDoubleSpinBoxWidget::AssetDoubleSpinBoxWidget(QWidget *parent, const std::string& val,
		double min, double max, double step) :
	QWidget(parent)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);

	// lin edit for path.
	pSpinBox_ = new QDoubleSpinBox();
	pSpinBox_->setRange(min, max);
	pSpinBox_->setSingleStep(step);
	pSpinBox_->setMinimumWidth(96);

	setValue(val);

	connect(pSpinBox_, SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)));
	pLayout->addWidget(pSpinBox_);
	pLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));

	setLayout(pLayout);
}

AssetDoubleSpinBoxWidget::~AssetDoubleSpinBoxWidget()
{
}


void AssetDoubleSpinBoxWidget::setValue(const std::string& strVal)
{
	double val = 0;
	if (!strVal.empty()) {
		val = std::stod(strVal);
	}

	pSpinBox_->blockSignals(true);
	pSpinBox_->setValue(val);
	pSpinBox_->blockSignals(false);
}


void AssetDoubleSpinBoxWidget::valueChanged(double val)
{
	std::string strVal = std::to_string(val);

	emit valueChanged(strVal);
}



// -------------------------------------------------


AssetSpinBoxWidget::AssetSpinBoxWidget(QWidget *parent, const std::string& val, double min, double max, double step) :
	QWidget(parent)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);

	// lin edit for path.
	pSpinBox_ = new QSpinBox();
	pSpinBox_->setRange(static_cast<int32_t>(math<double>::floor(min)), static_cast<int32_t>(math<double>::floor(max)));
	pSpinBox_->setSingleStep(static_cast<int32_t>(math<double>::floor(step)));
	pSpinBox_->setMinimumWidth(96);

	setValue(val);

	connect(pSpinBox_, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));

	pLayout->addWidget(pSpinBox_);
	pLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));

	setLayout(pLayout);
}

AssetSpinBoxWidget::~AssetSpinBoxWidget()
{
}

void AssetSpinBoxWidget::setValue(const std::string& strVal)
{
	int val = 0;
	if (!strVal.empty()) {
		val = std::stoi(strVal);
	}

	pSpinBox_->blockSignals(true);
	pSpinBox_->setValue(val);
	pSpinBox_->blockSignals(false);
}


void AssetSpinBoxWidget::valueChanged(int val)
{
	std::string strVal = std::to_string(val);

	emit valueChanged(strVal);
}


X_NAMESPACE_END