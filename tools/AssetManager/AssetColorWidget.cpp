#include "stdafx.h"
#include "AssetColorWidget.h"

#include "ColorSelectorWidget.h"

X_NAMESPACE_BEGIN(assman)


AssetColorWidget::AssetColorWidget(QWidget *parent, const std::string& value)
	: QWidget(parent),
	picking_(false)
{
	QHBoxLayout* pChildLayout = new QHBoxLayout();
	pChildLayout->setContentsMargins(0, 0, 0, 0);

	{
		pColPreview_ = new ColorSelector();
		pColPreview_->setMinimumWidth(64);
		pColPreview_->setDisplayMode(ColorPreview::DisplayMode::SplitAlpha); // it don't split if alpha solid.

		QToolButton* pButton = new QToolButton();
		pButton->setToolTip("Color Picker");
		pButton->setIcon(QIcon(":/misc/img/colorpicker.png"));

		connect(pButton, SIGNAL(pressed()), this, SLOT(colPickerClicked()));


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

			connect(pLineEdit, SIGNAL(editingFinished()), this, SLOT(editingFinished()));

			pChildLayout->addWidget(pLabel, 0);
			pChildLayout->addWidget(pLineEdit, 0);
		}
	}

	// set the values?
	// the string is floats space seperated.
	{
		QString tmp = QString::fromStdString(value);
		QStringList values = tmp.split(QChar(' '));

		while (values.size() < 4) {
			values.append("1");
		}

		QColor col;
		col.setRgbF(
			values[0].toFloat(),
			values[1].toFloat(),
			values[2].toFloat(),
			values[3].toFloat());

		for (int32_t i = 0; i < 4; i++) {
			pRGBAValueWidgets_[i]->blockSignals(true);
		}

		setColorInternal(col);

		for (int32_t i = 0; i < 4; i++) {
			pRGBAValueWidgets_[i]->blockSignals(false);
		}
	}

	setLayout(pChildLayout);
}

AssetColorWidget::~AssetColorWidget()
{
}



void AssetColorWidget::setColorInternal(QColor col)
{
	int32_t colors[4] = { col.red(), col.green(), col.blue(), col.alpha() };
	
	for (int32_t i = 0; i < 4; i++)	{
		pRGBAValueWidgets_[i]->setText(QString::number(colors[i]));
	}

	pColPreview_->setColor(col);
}

void AssetColorWidget::colPickerClicked(void)
{
	picking_ = true;
	grabMouse(Qt::CrossCursor);

	QPixmap pix(":/misc/img/colorpicker.png");
	QCursor cur(pix, 1, 15);
	QGuiApplication::setOverrideCursor(cur);
}

void AssetColorWidget::editingFinished(void)
{
	// you fucking edited one of the color line edits.
	QObject* pSender = sender();

	QColor col = pColPreview_->color();

	for (int32_t i = 0; i < 4; i++)
	{
		if (pSender == pRGBAValueWidgets_[i])
		{
			QString text = pRGBAValueWidgets_[i]->text();
			const int32_t newVal = text.toInt();

			switch (i)
			{
			case 0:
				col.setRed(newVal);
				break;
			case 1:
				col.setGreen(newVal);
				break;
			case 2:
				col.setBlue(newVal);
				break;
			case 3:
				col.setAlpha(newVal);
				break;
			}

			break;
		}
	}

	pColPreview_->setColor(col);
}

void AssetColorWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (picking_)
	{
		setColorInternal(getScreenColor(event->globalPos()));
		picking_ = false;
		releaseMouse();

		QGuiApplication::restoreOverrideCursor();
	}
}

void AssetColorWidget::mouseMoveEvent(QMouseEvent *event)
{
	X_UNUSED(event);

	if (picking_) {
	//	setColorInternal(getScreenColor(event->globalPos()));
	}
}

QColor AssetColorWidget::getScreenColor(const QPoint& globalPos)
{
	const int32_t screenNum = QApplication::desktop()->screenNumber(globalPos);
	QScreen *screen = QApplication::screens().at(screenNum);

	WId wid = QApplication::desktop()->winId();
	QImage img = screen->grabWindow(wid, globalPos.x(), globalPos.y(), 1, 1).toImage();

	return img.pixel(0, 0);
}

X_NAMESPACE_END