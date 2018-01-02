#include "stdafx.h"
#include "ColorSelectorWidget.h"

#include <QColorDialog>

X_NAMESPACE_BEGIN(editor)


class ColorSelector::Private
{
public:
	UpdateMode updateMode;
	QColorDialog* pDialog;
	QColor oldColor;

	Private(QWidget* widget) :
		pDialog(new QColorDialog(widget))
	{
		pDialog->setOption(QColorDialog::ShowAlphaChannel);
	}
};

ColorSelector::ColorSelector(QWidget *parent) :
	ColorPreview(parent), p(new Private(this))
{
	setUpdateMode(UpdateMode::Continuous);
	p->oldColor = color();

	connect(this, SIGNAL(clicked()), this, SLOT(showDialog()));
	connect(p->pDialog, SIGNAL(rejected()), this, SLOT(rejectDialog()));
	connect(p->pDialog, SIGNAL(colorSelected(const QColor&)), this, SLOT(dialogColorSelected(const QColor&)));

	setAcceptDrops(true);
}

ColorSelector::~ColorSelector()
{
	delete p;
}

ColorSelector::UpdateMode ColorSelector::updateMode(void) const
{
	return p->updateMode;
}

void ColorSelector::setUpdateMode(UpdateMode m)
{
	p->updateMode = m;
}

Qt::WindowModality ColorSelector::dialogModality(void) const
{
	return p->pDialog->windowModality();
}

void ColorSelector::setDialogModality(Qt::WindowModality m)
{
	p->pDialog->setWindowModality(m);
}


void ColorSelector::showDialog(void)
{
	p->oldColor = color();
	p->pDialog->setCurrentColor(color());

	connectDialog();

	p->pDialog->open();
}


void ColorSelector::connectDialog(void)
{
	if (p->updateMode == UpdateMode::Continuous) {
		connect(p->pDialog, SIGNAL(currentColorChanged(const QColor&)), this, SLOT(setColor(const QColor&)), Qt::UniqueConnection);
	}
	else {
		disconnectDialog();
	}
}

void ColorSelector::disconnectDialog(void)
{
	disconnect(p->pDialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(currentColorChanged(QColor)));
}

void ColorSelector::rejectDialog(void)
{
	setColor(p->oldColor);
}

void ColorSelector::dialogColorSelected(const QColor& c)
{
	setColor(c);
	p->oldColor = c;

	emit colorSelected(c);
}


void ColorSelector::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasColor() ||
		(event->mimeData()->hasText() && QColor(event->mimeData()->text()).isValid())) {
		event->acceptProposedAction();
	}
}


void ColorSelector::dropEvent(QDropEvent* event)
{
	if (event->mimeData()->hasColor())
	{
		setColor(event->mimeData()->colorData().value<QColor>());
		event->accept();
	}
	else if (event->mimeData()->hasText())
	{
		QColor col(event->mimeData()->text());
		if (col.isValid())
		{
			setColor(col);
			event->accept();
		}
	}
}

X_NAMESPACE_END