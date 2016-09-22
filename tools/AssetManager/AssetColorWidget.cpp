#include "stdafx.h"
#include "AssetColorWidget.h"

#include "ColorSelectorWidget.h"

X_NAMESPACE_BEGIN(assman)


AssetColorWidget::AssetColorWidget(QWidget *parent, const std::string& value)
	: QWidget(parent)
{
	QHBoxLayout* pChildLayout = new QHBoxLayout();
	pChildLayout->setContentsMargins(0, 0, 0, 0);

	{
		pColPreview_ = new ColorSelector();
		pColPreview_->setMinimumWidth(64);


		QToolButton* pButton = new QToolButton();
		pButton->setToolTip("Color Picker");
		pButton->setIcon(QIcon(":/misc/img/colorpicker.png"));

		pChildLayout->addWidget(pColPreview_);
		pChildLayout->addWidget(pButton);

		const char* pLabels[4] = { "R", "G", "B", "A" };

		for (int32_t i = 0; i < 4; i++)
		{
			QLabel* pLabel = new QLabel();
			pLabel->setText(pLabels[i]);

			QLineEdit* pLineEdit = pRGBAValueWidgets_[i] = new QLineEdit();
			pLineEdit->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
			pLineEdit->setMaximumWidth(64);
			pLineEdit->setValidator(new QIntValidator(0, 255));

			pChildLayout->addWidget(pLabel, 0);
			pChildLayout->addWidget(pLineEdit, 0);
		}
	}

	// set the values?
	// the string is floats space seperated.
	QString tmp = QString::fromStdString(value);
	QStringList values = tmp.split(QChar(' '));

	float vlaues[4] = { 1.f };
	int32_t i;

	for (i = 0; i < values.size(); i++) {
		vlaues[i] = values[i].toFloat();
	}
	for (; i < 4; i++) {
		vlaues[i] = 1.f;
	}

	for (i = 0; i < 4; i++)
	{
		pRGBAValueWidgets_[i]->blockSignals(true);
		pRGBAValueWidgets_[i]->setText(QString::number(math<float>::floor(vlaues[i] * 255.f)));
		pRGBAValueWidgets_[i]->blockSignals(false);
	}

	{
		QColor col;
		col.setRgbF(vlaues[0], vlaues[1], vlaues[2], vlaues[3]);

		pColPreview_->setColor(col);
		pColPreview_->setDisplayMode(ColorPreview::DisplayMode::SplitAlpha); // it don't split if alpha solid.
	}

	setLayout(pChildLayout);
}

AssetColorWidget::~AssetColorWidget()
{
}

X_NAMESPACE_END