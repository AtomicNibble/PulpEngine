#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(editor)


class WindowButton : public QPushButton
{
	Q_OBJECT

	enum ButtonState
	{
		STATE_NORMAL,
		STATE_HOVERED,
		STATE_CLICKED
	};

public:
	enum ButtonType
	{
		BUTTON_MINIMIZE, 
		BUTTON_MAXIMIZE, 
		BUTTON_CLOSE
	};
public:

	WindowButton(ButtonType type, QWidget *parent = 0);
	~WindowButton();

protected:
	void resizeEvent(QResizeEvent* event);
	void paintEvent(QPaintEvent* event);

	void enterEvent(QEvent* event);
	void leaveEvent(QEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);

private:
	void InitPixmaps(const QSize& size);
	void InitPixmap(QPixmap **pixmap);
	void InitMinimize(const QSize& size);
	void InitMaximize(const QSize& size);
	void InitClose(const QSize& size);

private:
	ButtonType  type_;
	ButtonState state_;
	QPixmap* pNormal_;
	QPixmap* pHovered_;
	QPixmap* pClicked_;

};


X_NAMESPACE_END