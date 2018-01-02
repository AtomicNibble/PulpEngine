#include "stdafx.h"
#include "WindowTileBar.h"
#include "WindowButton.h"

#include <QPixmap>

X_NAMESPACE_BEGIN(editor)


namespace
{

	void StyleButton(QAbstractButton* button)
	{
		button->setFixedSize(34, 26);
		button->setStyleSheet(
			"margin: 0px;"
		);
	}

} // namespace

class WindowIcon : public QWidget
{
public:
	WindowIcon(QWidget* parent = 0) :
		QWidget(parent), 
		hasfocus_(true)
	{
	
	}

	void SetPix(const QPixmap& pix) 
	{
		icon_.addPixmap(pix);

		// genrate ddisabled version.
		QPixmap disabledPixmap(pix.size());
		disabledPixmap.fill(Qt::transparent);
		QPainter p(&disabledPixmap);

		p.setBackgroundMode(Qt::TransparentMode);
		p.setBackground(QBrush(Qt::transparent));
		p.eraseRect(pix.rect());

		p.setOpacity(0.5);
		p.drawPixmap(0, 0, pix);

		p.end();
		icon_.addPixmap(disabledPixmap, QIcon::Disabled);
	}

	void Focus(bool f) 
	{
		//    qDebug() << "Focus: " << f;
		hasfocus_ = f;
		repaint();
	}

private:
	void paintEvent(QPaintEvent * event)
	{
		X_UNUSED(event);

		QPainter painter(this);
		painter.drawPixmap(8, 6, 24, 24, icon_.pixmap(24, 24,
			hasfocus_ ? QIcon::Normal : QIcon::Disabled));
	}

protected:
	QIcon icon_;
	bool  hasfocus_;
};


// -----------------------------------------

WindowTitleBar::WindowTitleBar(QWidget *parent) :
	QWidget(parent), 
	pTitle_(nullptr), 
	move_(false)
{
	setFixedHeight(34);

	UpdateWindowTitle();

	QHBoxLayout* hbox = new QHBoxLayout(this);


	QImage image(":/misc/img/logo_64.png");
	QPixmap pix = QPixmap::fromImage(image);


	pIcon_ = new WindowIcon(this);
	pIcon_->SetPix(pix);
	pIcon_->setMaximumWidth(36);
	pIcon_->setStyleSheet("padding-left:8px;padding-top:6px;");

	pTitle_ = new QLabel(this);
	pTitle_->setStyleSheet("color:#999999; font-family:No; font-size:12px; margin:3px; padding-top:3px");

	pMinimize_ = new WindowButton(WindowButton::ButtonType::BUTTON_MINIMIZE, this);
	pMaximize_ = new WindowButton(WindowButton::ButtonType::BUTTON_MAXIMIZE, this);
	pClose_ = new WindowButton(WindowButton::ButtonType::BUTTON_CLOSE, this);

	StyleButton(pMinimize_);
	StyleButton(pMaximize_);
	StyleButton(pClose_);

	hbox->setSpacing(0);
	hbox->setMargin(0);
	hbox->addWidget(pIcon_);
	hbox->addWidget(pTitle_);
	hbox->addWidget(pMinimize_, 0, Qt::AlignTop);
	hbox->addWidget(pMaximize_, 0, Qt::AlignTop);
	hbox->addWidget(pClose_, 0, Qt::AlignTop);
	//   hbox->insertStretch(1, 500);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	setLayout(hbox);

	connect(pMinimize_, SIGNAL(clicked()),
		this, SLOT(Minimized()));

	connect(pMaximize_, SIGNAL(clicked()),
		this, SLOT(Maximized()));

	connect(pClose_, SIGNAL(clicked()),
		this, SLOT(Quit()));

}

WindowTitleBar::~WindowTitleBar()
{

}



void WindowTitleBar::Focus(bool f)
{
	pIcon_->Focus(f);
}

void WindowTitleBar::UpdateWindowTitle()
{
	if (pTitle_)
		pTitle_->setText(window()->windowTitle());
}


void WindowTitleBar::resizeEvent(QResizeEvent * event)
{
	Q_UNUSED(event);

}

void WindowTitleBar::paintEvent(QPaintEvent  * event)
{
	Q_UNUSED(event);

	// if (m_Cache != NULL)
	{
		//   QPainter painter(this);
		//   painter.drawPixmap(0, 0, *m_Cache);
	}
}


void WindowTitleBar::mousePressEvent(QMouseEvent *e)
{
	if (e->pos().x() >= PIXELS_TO_ACT && e->pos().x() < geometry().width()
		&& e->pos().y() >= PIXELS_TO_ACT && e->pos().y() < geometry().height())
	{
		diff_ = e->pos();
		move_ = true;

		qDebug() << "In move area";
	}
}

void WindowTitleBar::mouseReleaseEvent(QMouseEvent *event)
{
	Q_UNUSED(event);
	move_ = false;
	//  setCursor(QCursor(Qt::ArrowCursor));
}

void WindowTitleBar::mouseMoveEvent(QMouseEvent *event)
{
	if (move_) {

		if (window()->windowState() == Qt::WindowMaximized)
		{
			window()->showNormal();
			diff_ = window()->pos();
		}

		QPoint p = event->globalPos();

		window()->move(p - diff_);
	}
}


void WindowTitleBar::mouseDoubleClickEvent(QMouseEvent* e)
{
	if (e->pos().x() < geometry().width()
		&& e->pos().y() < geometry().height())
	{
		Maximized();
	}

	e->accept();
}


void WindowTitleBar::Minimized()
{
	window()->showMinimized();
}

void WindowTitleBar::Maximized()
{
	if (window()->windowState() == Qt::WindowMaximized)
	{
		window()->showNormal();
	}
	else
	{
		window()->showMaximized();
	}
}

void WindowTitleBar::Quit()
{
	window()->close();
}



X_NAMESPACE_END