#include "stdafx.h"
#include "AssetColorWidget.h"

X_NAMESPACE_BEGIN(assman)


AssetColorWidget::AssetColorWidget(QWidget *parent, const std::string& value)
	: QWidget(parent)
{
	QHBoxLayout* pChildLayout = new QHBoxLayout();
	pChildLayout->setContentsMargins(0, 0, 0, 0);

	{
		QLabel* pColLabel = new QLabel();
		pColLabel->setFrameStyle(QFrame::Box | QFrame::Panel | QFrame::Plain | QFrame::Raised);
		pColLabel->setAlignment(Qt::AlignCenter);
		pColLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		pColLabel->setMinimumWidth(64);

		pChildLayout->addWidget(pColLabel);

		QToolButton* pButton = new QToolButton();
		pButton->setToolTip("Color Picker");
		pButton->setIcon(QIcon(":/misc/img/colorpicker.png"));

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

	float vlaues[4] = { 255.f };

	for (int32_t i = 0; i < values.size(); i++)
	{
		vlaues[i] = values[i].toFloat();
		vlaues[i] = math<float>::floor(vlaues[i] * 255.f);
	}

	for (int32_t i = 0; i < 4; i++)
	{
		pRGBAValueWidgets_[i]->blockSignals(true);
		pRGBAValueWidgets_[i]->setText(QString::number(vlaues[i]));
		pRGBAValueWidgets_[i]->blockSignals(false);
	}

	setLayout(pChildLayout);
}

AssetColorWidget::~AssetColorWidget()
{
}

X_NAMESPACE_END