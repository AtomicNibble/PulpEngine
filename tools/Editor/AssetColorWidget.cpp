#include "stdafx.h"
#include "AssetColorWidget.h"

#include "ColorSelectorWidget.h"

X_NAMESPACE_BEGIN(editor)


AssetColorWidget::AssetColorWidget(QWidget *parent, const std::string& value) :
	QWidget(parent),
	picking_(false)
{
	QHBoxLayout* pChildLayout = new QHBoxLayout();
	pChildLayout->setContentsMargins(0, 0, 0, 0);

	{
		pColPreview_ = new ColorSelector();
		pColPreview_->setMinimumWidth(64);
		pColPreview_->setDisplayMode(ColorPreview::DisplayMode::SplitAlpha); // it don't split if alpha solid.
		pColPreview_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

		connect(pColPreview_, SIGNAL(colorSelected(const QColor&)), this, SLOT(colorSelected(const QColor&)));

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
			pLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

			QLineEdit* pLineEdit = pRGBAValueWidgets_[i] = new QLineEdit();
			pLineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
			pLineEdit->setMaximumWidth(64);
			pLineEdit->setValidator(new QIntValidator(0, 255));

			connect(pLineEdit, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
			connect(pLineEdit, SIGNAL(textEdited(const QString &)), this, SLOT(validateText(const QString &)));

			pChildLayout->addWidget(pLabel, 0);
			pChildLayout->addWidget(pLineEdit, 0);
		}
	}

	pChildLayout->addSpacerItem(new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Expanding));

	// set the values?
	// the string is floats space seperated.
	setValue(value);

	setLayout(pChildLayout);
}

AssetColorWidget::~AssetColorWidget()
{
}


void AssetColorWidget::setValue(const std::string& value)
{
	QString tmp = QString::fromStdString(value);
	QStringList values = tmp.split(QChar(' '));

	while (values.size() < 4) {
		values.append("1");
	}

	QColor col;
	col.setRgb(
		static_cast<int32_t>(values[0].toDouble() * 255),
		static_cast<int32_t>(values[1].toDouble() * 255),
		static_cast<int32_t>(values[2].toDouble() * 255),
		static_cast<int32_t>(values[3].toDouble() * 255)
	);

	blockSignals(true);
	for (int32_t i = 0; i < 4; i++) {
		pRGBAValueWidgets_[i]->blockSignals(true);
	}

	setColorInternal(col, true);

	for (int32_t i = 0; i < 4; i++) {
		pRGBAValueWidgets_[i]->blockSignals(false);
	}
	blockSignals(false);
}

void AssetColorWidget::setColorInternal(QColor col, bool force)
{
	if (!force && curCol_ == col) {
		return;
	}

	curCol_ = col;

	int32_t colors[4] = { col.red(), col.green(), col.blue(), col.alpha() };
	
	for (int32_t i = 0; i < 4; i++)	{
		pRGBAValueWidgets_[i]->setText(QString::number(colors[i]));
	}

	pColPreview_->setColor(col);

	// back to string xD
	QString temp = QString("%1 %2 %3 %4").arg(
		QString::number(static_cast<double>(colors[0]) / 255, 'f', 4),
		QString::number(static_cast<double>(colors[1]) / 255, 'f', 4),
		QString::number(static_cast<double>(colors[2]) / 255, 'f', 4),
		QString::number(static_cast<double>(colors[3]) / 255, 'f', 4)
	);

	emit valueChanged(temp.toStdString());
}

// color selector changed.
void AssetColorWidget::colorSelected(const QColor& col)
{
	setColorInternal(col);
}

// show the color picker
void AssetColorWidget::colPickerClicked(void)
{
	picking_ = true;
	grabMouse(Qt::CrossCursor);

	QPixmap pix(":/misc/img/colorpicker.png");
	QCursor cur(pix, 1, 15);
	QGuiApplication::setOverrideCursor(cur);
}

// one of the 4 line edits changed
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

	setColorInternal(col);
}

void AssetColorWidget::validateText(const QString& text)
{
	QObject* pSender = sender();

	// i could just cast pSender to QlineEdit and set text.
	// but i did this as I potentially might want to update the saved color, so i need to know the channel.
	if (text.isEmpty())
	{
		for (int32_t i = 0; i < 4; i++)
		{
			if (pSender == pRGBAValueWidgets_[i])
			{
				pRGBAValueWidgets_[i]->setText("0");
				break;
			}
		}
	}
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
	//  if want live update.
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